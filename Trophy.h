#pragma once

#include "DoublyLinkedList.h"
#include "Util.h"
#include "WinUtil.h"
#include <fstream>
#include <string>

static const std::wstring trophy_prefixes[16]{
    L"Rev Limit",
    L"Dyno",
    L"Intake Manifold",
    L"Octane",
    L"Piston",
    L"Drag Coefficient",
    L"Forced Induction",
    L"Throttle Body",
    L"G-Force",
    L"Horsepower",
    L"V12",
    L"Powertrain",
    L"Slip Differential",
    L"Turbocharger",
    L"Supercharger",
    L"Carbon Fibre",
};

static const std::wstring trophy_suffixes[16]{
    L"Badge",      L"Trophy", L"Collectible", L"Keepsake", L"Cup",   L"Medal",
    L"Prize",      L"Award",  L"Laurel",      L"Memento",  L"Token", L"Ribbon",
    L"Decoration", L"Star",   L"Shield",      L"Accolade",
};

static constexpr auto trophy_filename = "trophy.bin";

// High four bites represent prefix, low four bits represent suffix
std::wstring get_trophy_string(char trophy);

void add_trophy();

DoublyLinkedList<std::wstring> get_trophies();
