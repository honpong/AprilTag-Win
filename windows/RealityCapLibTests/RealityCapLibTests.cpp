// RealityCapLibTests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../gtest/gtest.h"

GTEST_API_ int main(int argc, char **argv) {
    printf("Running main() from gtest_main.cc\n");
    testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    if (result)
    {
        std::cout << "\nPress enter to exit.\n";
        std::cin.get();
    }
    return result;
}