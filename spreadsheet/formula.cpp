#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

FormulaError::FormulaError(Category category)
    : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return ToString() == rhs.ToString();
}

std::string_view FormulaError::ToString() const {
    switch (category_)
    {
    case FormulaError::Category::Ref:
        return "#REF!";
    case FormulaError::Category::Value:
        return "#VALUE!";
    case FormulaError::Category::Div0:
        return "#DIV/0!";
    default:
        assert(false);
        return "";
    }
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression) 
            : ast_(ParseFormulaAST(expression)) {
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            Value result;
            try {
                result = ast_.Execute(sheet);
            }
            catch (const FormulaError& evaluate_error) {
                result = evaluate_error;
                //std::cout << "Inside catch part  " << evaluate_error << std::endl;
            }
            return result;
        }

        std::string GetExpression() const override {
            std::ostringstream outstr;
            ast_.PrintFormula(outstr);
            return outstr.str();
        }

        std::vector<Position> GetReferencedCells() const override {
            std::forward_list<Position> temp_list = ast_.GetCells();
            std::set<Position> temp_set(temp_list.begin(), temp_list.end());
            std::vector<Position> result(std::make_move_iterator(temp_set.begin()),
                                         std::make_move_iterator(temp_set.end()));
            return result;
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    }
    catch (...) {
        throw FormulaException("Uncorrect formula!");
    }
}