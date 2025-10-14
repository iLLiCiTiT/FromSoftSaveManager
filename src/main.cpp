#include <iostream>
#include <ostream>

#include "parse/Parse.h"

#include <string>

int main(int argc, char *argv[]) {
    const char *input_sl2_file_dsr = "/my/test/file/DRAKS0005.sl2";
    fsm::parse::SL2File sl2_dsr = fsm::parse::parse_sl2_file(input_sl2_file_dsr);
    return 0;
}