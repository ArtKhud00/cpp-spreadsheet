#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <string>
#include <optional>
#include <unordered_set>
#include <stack>

// ---------- Impl ------------------

class Cell::Impl {
public:
	virtual Value GetValue() const = 0;

	virtual std::string GetText() const = 0;

	virtual std::vector<Position> GetReferencedCells() const {
		return {};
	}

	virtual void CacheInvalidate() {}

	// check it 
	virtual bool IsReferenced() const {
		return false;
	}

	virtual ~Impl() = default;
};

// ---------- EmptyImpl ------------------

class Cell::EmptyImpl : public Impl {
public:
	Value GetValue() const override;

	std::string GetText() const override;
};

// ---------- TextImpl ------------------

class Cell::TextImpl : public Impl {
public:
	explicit TextImpl(std::string expression);

	Value GetValue() const override;

	std::string GetText() const override;
private:
	std::string text_;
};

// ---------- FormulaImpl ------------------

class Cell::FormulaImpl : public Impl {
public:
	explicit FormulaImpl(std::string expression, SheetInterface& sheet);

	Value GetValue() const override;

	std::string GetText() const override;

	std::vector<Position> GetReferencedCells() const override;

	//bool IsReferenced() const override;

	void CacheInvalidate() override;

private:
	std::unique_ptr<FormulaInterface> formula_;
	SheetInterface& sheet_;
	mutable std::optional<FormulaInterface::Value> cache_;
};


// ---------- EmptyImpl ------------------

Cell::Value Cell::EmptyImpl::GetValue() const {
	return "";
}

std::string Cell::EmptyImpl::GetText() const {
	return "";
}

// ---------- TextImpl ------------------

Cell::TextImpl::TextImpl(std::string expression)
	: text_(std::move(expression)) {
}

Cell::Value Cell::TextImpl::GetValue() const {
	if (text_.at(0) == ESCAPE_SIGN) {
		return text_.substr(1);
	}
	return text_;
}

std::string Cell::TextImpl::GetText() const {
	return text_;
}

// ---------- FormulaImpl ------------------

Cell::FormulaImpl::FormulaImpl(std::string expression, SheetInterface& sheet)
	//: formula_(ParseFormula(expression.substr(1)))
	: sheet_(sheet) {
	formula_ = ParseFormula(expression.substr(1));
}

Cell::Value Cell::FormulaImpl::GetValue() const {
	FormulaInterface::Value temp_value;

	if (!cache_.has_value()) {
		cache_= formula_->Evaluate(sheet_);
	}
	temp_value = cache_.value();

	if (std::holds_alternative<double>(temp_value)) {
		return std::get<double>(temp_value);
	}
	else {
		return std::get<FormulaError>(temp_value);
	}
}

std::string Cell::FormulaImpl::GetText() const {
	return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return formula_->GetReferencedCells();
}

void Cell::FormulaImpl::CacheInvalidate() {
	cache_.reset();
}

// ---------- Cell ------------------

Cell::Cell(Sheet& sheet)
	: impl_(std::make_unique<EmptyImpl>())
	, sheet_(sheet) {
}

Cell::~Cell() {
	impl_.release();
}

void Cell::Set(std::string text) {
	std::unique_ptr<Impl> temp_cell;

	if (text.empty()) {
		temp_cell = std::make_unique<Cell::EmptyImpl>();
	}
	else if (text.at(0) == FORMULA_SIGN && text.size() > 1) {
		temp_cell = std::make_unique<Cell::FormulaImpl>(std::move(text),sheet_);
	}
	else {
		temp_cell = std::make_unique<TextImpl>(std::move(text));
	}
	CheckCircularDependendency(*temp_cell);
	impl_ = std::move(temp_cell);
	for (const auto& pos : impl_->GetReferencedCells()) {
		Cell* cell = sheet_.GetConcreteCell(pos);
		if (!cell) {
			sheet_.SetCell(pos, "");
			cell = sheet_.GetConcreteCell(pos);
		}
		cell->dependent_cells_.insert(this);
	}
	CacheInvalidate();
}

void Cell::Clear() {
	//impl_ = std::move(std::make_unique<EmptyImpl>());
	//impl_ = nullptr;
	//impl_.release();
	impl_.reset(nullptr);
	//impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
	return impl_->IsReferenced();
}

void Cell::CheckCircularDependendency(const Impl& impl) const {
	std::unordered_set<const Cell*> referenced_cells_set;
	for (const Position& cell_pos : impl.GetReferencedCells()) {
		referenced_cells_set.insert(sheet_.GetConcreteCell(cell_pos));
	}
	std::stack<const Cell*> queue_to_visit;
	std::unordered_set<const Cell*> already_visited;
	queue_to_visit.push(this);
	while (!queue_to_visit.empty()) {
		const Cell* curr = queue_to_visit.top();
		queue_to_visit.pop();
		if (referenced_cells_set.count(curr) > 0) {
			throw CircularDependencyException("");
		}
		already_visited.insert(curr);
		for (const Cell* cell : curr->dependent_cells_) {
			if (already_visited.count(cell) == 0) {
				queue_to_visit.push(cell);
			}
		}
	}

}

void Cell::CacheInvalidate() {
	impl_->CacheInvalidate();
	for (Cell* cell : dependent_cells_) {
		cell->CacheInvalidate();
	}
}