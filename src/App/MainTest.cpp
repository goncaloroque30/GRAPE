// Copyright (C) 2023 Goncalo Soares Roque 

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"

int main(int ArgCount, char** ArgValues) {
    GRAPE::initGRAPE();
    doctest::Context context(ArgCount, ArgValues);
#ifndef GRAPE_DEBUG
    context.setOption("--no-breaks", true);
#endif

    int res = context.run();
    if (context.shouldExit())
        return res;

    return res;
}
