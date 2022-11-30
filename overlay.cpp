//
// Created by Sidney Goldinger on 11/30/22.
//

#import <stdio.h>
#include <iostream>
#include <string_view>

void router() {
    printf("router\n");
}

void endhost() {
    printf("host\n");
}

int main (int argc, char **argv) {
    // test if end-host or router
    std::string arg1 = argv[1];

    if (arg1 == "r") { router(); }
    else if (arg1 == "h") { endhost(); }
    else { printf("please input r or h.\n"); return 0; }

    return 0;
}
