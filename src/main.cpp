#include <iostream>
#include <ostream>

#include "parse/Parse.h"
#include "parse/DSRSaveFile.h"

#include <string>

int main(int argc, char *argv[]) {
    const char *input_sl2_file_dsr = "/my/test/file/DRAKS0005.sl2";
    fsm::parse::SL2File sl2_dsr = fsm::parse::parse_sl2_file(input_sl2_file_dsr);
    std::cout << gameToString(sl2_dsr.game) << std::endl;
    switch (sl2_dsr.game) {
        case fsm::parse::Game::DSR:
            fsm::parse::DSRSaveFile dsr = fsm::parse::parse_dsr_file(sl2_dsr);
            break;
    }
    return 0;
}