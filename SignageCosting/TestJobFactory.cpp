#include "TestJobFactory.h"
#include "MaterialDatabase.h"
#include <iostream>

static void applyDefaultProduction(JobItem& item)
{
    item.production.print = true;
    item.production.laminate = true;
    item.production.plotterCut = true;
    item.production.routerCut = true;
    item.production.application = true;
    item.production.frame = false;
}

Job TestJobFactory::createDefaultTestJob()
{
    Job job;

    // ---------------- VINYL ----------------
    JobItem vinylItem;
    vinylItem.material = MaterialDatabase::get("VINYL");
    vinylItem.width = 300;
    vinylItem.height = 300;
    vinylItem.quantity = 5;
    vinylItem.variantIndex = 4;

    const auto& v = vinylItem.material.variants[vinylItem.variantIndex];
    vinylItem.selectedRollWidth =
        v.roll_widths.empty() ? 0 : v.roll_widths[0];

    applyDefaultProduction(vinylItem);

    job.addItem(vinylItem);

    // ---------------- CHROMADEK ----------------
    JobItem chromadekItem;
    chromadekItem.material = MaterialDatabase::get("CHROMADEK");
    chromadekItem.width = 500;
    chromadekItem.height = 500;
    chromadekItem.quantity = 5;
    chromadekItem.variantIndex = 2;

    applyDefaultProduction(chromadekItem);

    job.addItem(chromadekItem);

    return job;
}