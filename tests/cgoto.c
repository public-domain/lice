// labels as values (computed goto)

int main(void) {
    // implement a small dispatch table of instructions for a mini
    // state machine, to test computed goto

    // test state
    int instruction_0_state = 0;
    int instruction_1_state = 0;
    int instruction_2_state = 0;

    unsigned char bytecode[] = {0x01, 0x05, 0x01, 0x02, 0x02, 0x03};
    unsigned char *pc        = bytecode;

    void *dispatch[] = {
        0,
        &&instruction_0,
        &&instruction_1,
        &&instruction_2
    };

    #define DISPATCH(INDEX)      \
        do {                     \
            pc += (INDEX);       \
            goto *dispatch[*pc]; \
        } while (0)

    // machine loop
    do {

        instruction_0:
            instruction_0_state += *(pc + 1);
            DISPATCH(2);

        instruction_1:
            instruction_1_state ++;
            DISPATCH(1);

        instruction_2:
            instruction_2_state --;
            break;

    } while (1);

    expecti(instruction_0_state, 7);
    expecti(instruction_1_state, 1);
    expecti(instruction_2_state, -1);

    return 0;
}
