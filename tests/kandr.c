// K&R C

#pragma warning_disable

// implicit integer
function() {
    return 4;
}

int main(void) {
    expecti(function(), 4);
    expecti(defined_later(), 1337);
}

defined_later() {
    // this is defined later, but main can call into it
    // since K&R doesn't care about declarations

    return 1337;
}

#pragma warning_enable
