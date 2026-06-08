#include "InvoicePrinter.h"
#include "Format.h"
#include <iostream>

void InvoicePrinter::print(const CostResult& result)
{
    std::cout << "\n========================\n";
    std::cout << "INVOICE BREAKDOWN\n";
    std::cout << "========================\n\n";

    for (const auto& item : result.items)
    {
        std::cout << "Item: " << item.materialId << " [" << item.category << "]\n";
        std::cout << "Area: " << item.area << " m2\n";

        std::cout << "Material Cost: " << Format::money(item.materialCost) << "\n";
        std::cout << "Labour Cost: " << Format::money(item.labourCost) << "\n";

        std::cout << "Markup Value: " << Format::money(item.markupValue) << "\n";

        std::cout << "------------------------\n";
        std::cout << "Sell Price: " << Format::money(item.sellPrice) << "\n";
        std::cout << "------------------------\n\n";
    }

    std::cout << "=============================\n";
    std::cout << " TOTAL SELL PRICE: " << Format::money(result.sellPrice) << "\n";
    std::cout << "=============================\n";
}