#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <sstream>

using namespace std::literals;

Sheet::~Sheet() {}

struct ValuePrinter {
    std::ostream& out;

    void operator()(std::string value_text) const {
        out << value_text;
    }
    void operator()(double value_number) const {
        out << value_number;
    }
    void operator()(FormulaError fe) const {
        out << fe;
    }
};


std::ostream& operator<<(std::ostream& out, CellInterface::Value val) {
    std::ostringstream strm;
    std::visit(ValuePrinter{ strm }, val);
    out << strm.str();
    return out;
}

void Sheet::SetCell(Position pos, std::string text) {
    if (pos.IsValid()) {
        if (static_cast<size_t>(pos.row) >= std::size(cells_)) {
            cells_.resize(pos.row == 0 ? 2 : pos.row * 2);
            for (size_t i = 0; i < cells_.size(); ++i) {
                cells_[i].resize(cells_[0].size());
            }
        }
        if (static_cast<size_t>(pos.col) >= cells_[0].size()) {
            for (size_t i = 0; i < std::size(cells_); ++i) {
                cells_[i].resize(pos.col == 0 ? 2 : pos.col * 2);
            }
        }
        if (!cells_[pos.row][pos.col]) {
            cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);
        }
        cells_[pos.row][pos.col]->Set(std::move(text));
        UpdatePrintableSize(pos, 's');
    }
    else {
        throw InvalidPositionException("invalid position. SetCell method");
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (pos.IsValid()) {
        if (pos.row < int(std::size(cells_)) && pos.col < int(std::size(cells_[pos.row]))) {
            return cells_[pos.row][pos.col].get();

        }
        else {
            return nullptr;
        }
    }
    else {
        throw InvalidPositionException("invalid position. GetCell method");
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if (pos.IsValid()) {
        if (pos.row < int(std::size(cells_)) && pos.col < int(std::size(cells_[pos.row]))) {
            return cells_[pos.row][pos.col].get();
        }
        else {
            return nullptr;
        }
    }
    else {
        throw InvalidPositionException("invalid position. GetCell method");
    }
}


void Sheet::ClearCell(Position pos) {

    if (pos.IsValid()) {
        if (pos.row < int(std::size(cells_)) && pos.col < int(std::size(cells_[pos.row]))) {
            if (cells_[pos.row][pos.col]) {
                cells_[pos.row][pos.col]->Clear();
                cells_[pos.row][pos.col].reset(nullptr);
            }
            UpdatePrintableSize(pos, 'c');
        }
    }
    else {
        throw InvalidPositionException("invalid cell position. clearcell");
    }
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        bool is_first = true;
        for (int j = 0; j < printable_size_.cols; ++j) {
            CellInterface::Value temp_buffer = "";
            if (cells_[i][j]) {
                temp_buffer = cells_[i][j]->GetValue();
            }
            if (is_first) {
                output << temp_buffer;
                is_first = false;
            }
            else {
                output << "\t" << temp_buffer;
            }
        }
        output << "\n";
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        bool is_first = true;
        for (int j = 0; j < printable_size_.cols; ++j) {
            std::string temp_buffer;
            if (cells_[i][j]) {
                temp_buffer = cells_[i][j]->GetText();
            }
            if (is_first) {
                output << temp_buffer;
                is_first = false;
            }
            else {
                output << "\t" << temp_buffer;
            }
        }
        output << "\n";
    }
}

void Sheet::UpdatePrintableSize(Position pos, char mode) {
    
    if (mode == 's') {
        if (printable_size_.rows < pos.row + 1) {
            printable_size_.rows = pos.row + 1;
        }
        if (printable_size_.cols < pos.col + 1) {
            printable_size_.cols = pos.col + 1;
        }
        return;
    }
    // удаление крайних элементов, находящихся на границе печатаемой области
    // находим новую горизонтальную границу
    if (mode == 'c') {
        if (printable_size_.rows == pos.row + 1) {
            bool done = false;
            for (int i = printable_size_.rows - 1; i >= 0 && !done; --i) {
                for (int j = printable_size_.cols - 1; j >= 0; --j) {
                    if (cells_[i][j]) {
                        printable_size_.rows = i + 1;
                        done = true;
                        break;
                    }
                    // случай, когда удаляем единственный элемент в левом верхнем углу
                    if (i == 0 && j == 0 && !cells_[i][j]) {
                        printable_size_ = { 0,0 };
                        return;
                    }
                }
            }
        }
        // находим новую вертикальную границу
        if (printable_size_.cols == pos.col + 1) {
            bool done = false;
            for (int j = printable_size_.cols - 1; j >= 0 && !done; --j) {
                for (int i = printable_size_.rows - 1; i >= 0; --i) {
                    if (cells_[i][j]) {
                        printable_size_.cols = j + 1;
                        done = true;
                        break;
                    }
                }
            }
        }
    }
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (pos.IsValid()) {
        if (pos.row < int(std::size(cells_)) && pos.col < int(std::size(cells_[pos.row]))) {
            return cells_[pos.row][pos.col].get();

        }
        else {
            return nullptr;
        }
    }
    else {
        throw InvalidPositionException("invalid position. GetCell method");
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}