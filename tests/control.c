// control flow

int if1(void) {
    if (1) {
        return 1;
    }
    return 0;
}

int if2(void) {
    if (0) {
        return 0;
    }
    return 2;
}

int if3(void) {
    if (1) {
        return 3;
    } else {
        return 0;
    }
    return 0;
}

int if4(void) {
    if (0) {
        return 0;
    } else {
        return 4;
    }
    return 0;
}

int if5(void) {
    if (1) return 5;
    return 0;
}

int if6(void) {
    if (0) return 0;
    return 6;
}

int if7(void) {
    if (1) return 7;
    else return 0;
    return 0;
}

int if8(void) {
    if (0) return 0;
    else return 8;
    return 0;
}

int if9(void) {
    // 0.5 evaluates true
    if (0.5) return 9;
    else return 0;
}

int main(void) {
    int i;
    int j;

    expecti(if1(), 1);
    expecti(if2(), 2);
    expecti(if3(), 3);
    expecti(if4(), 4);
    expecti(if5(), 5);
    expecti(if6(), 6);
    expecti(if7(), 7);
    expecti(if8(), 8);
    expecti(if9(), 9);

    // for control
    j = 0;
    for (i = 0; i < 5; i++)
        j = j + i;
    expecti(j, 10);

    j = 0;
    for (i = 0; i < 5; i++) {
        j = j + i;
    }
    expecti(j, 10);

    j = 0;
    for (i = 0; i < 100; i++) {
        if (i < 5)
            continue;
        if (i == 9)
            break;
        j += i;
    }
    expecti(j, 5 + 6 + 7 + 8);

    i = 0;
    for (; 0.5;) {
        i = 1337;
        break;
    }
    expecti(i, 1337);

    // while control
    i = 0;
    j = 0;
    while (i <= 100)
        j = j + i++;
    expecti(j, 5050);

    i = 0;
    j = 1;
    while (i <= 100)
        j = j + i++;
    expecti(j, 5051);

    i = 0;
    j = 0;
    while (i < 10) {
        if (i++ < 5)
            continue;
        j += i;
        if (i == 9)
            break;
    }
    expecti(j, 6 + 7 + 8 + 9);

    i = 0;
    while (0.5) {
        i = 1337;
        break;
    }
    expecti(i, 1337);

    // do control
    i = 0;
    j = 0;
    do {
        j = j + i++;
    } while (i <= 100);
    expecti(j, 5050);

    i = 0;
    do { i = 1337; } while (0);
    expecti(i, 1337);

    i = 0;
    j = 0;
    do {
        if (i++ < 5)
            continue;
        j += i;
        if (i == 9)
            break;
    } while (i < 10);
    expecti(j, 6 + 7 + 8 + 9);
    expecti(i, 9);

    i = 0;
    do { i++; break; } while (0.5);
    expecti(i, 1);

    // switch control
    i = 0;
    switch (1 + 2) {
        case 0:
            expecti(0, 1);
        case 3:
            i = 3;
            break;
        case 1:
            expecti(0, 1);
    }
    expecti(i, 3);

    i = 0;
    switch(1) {
        case 0: i++;
        case 1: i++;
        case 2: i++;
        case 3: i++;
    }
    expecti(i, 3);

    i = 0;
    switch (1024) {
        case 0: i++;
        default:
            i = 128;
    }
    expecti(i, 128);

    i = 0;
    switch (1024) {
        case 0: i++;
    }
    expecti(i, 0);

    i = 1024;
    switch (3) {
        i++;
    }

    switch (1337) {
        case 1 ... 100:
            expecti(1, 0);
        case 101:
            expecti(1, 0);
        case 102 ... 1337:
            break;
        default:
            expecti(1, 0);
    }

    expecti(i, 1024);

    i = 0;
    j = 1024;
    switch (j % 8) {
        case 0: do { i++;
        case 7:      i++;
        case 6:      i++;
        case 5:      i++;
        case 4:      i++;
        case 3:      i++;
        case 2:      i++;
        case 1:      i++;
                } while ((j -= 8) > 0);
    }
    expecti(i, 1024);

    // goto control
    j = 0;
    goto A;
    j = 5;
A:  expecti(j, 0);
    i = 0;
    j = 0;
B:  if (i > 10)
        goto C;
    j += i++;
    goto B;
C:  if (i > 11)
        goto D;
    expecti(j, 55);
    i++;
    goto B;
D:
    expecti(i, 12);
    expecti(j, 55);

    // logical or control flow
    expecti(0 || 3, 1);
    expecti(5 || 0, 1);
    expecti(0 || 0, 0);

    // empty expressions
    for (;;)
        break;

    for (i = 0; i < 50; i++)
        ;

    i = 0;
    while (i++ < 50)
        ;

    i = 0;
    do 1; while (i++ < 50);
    i = 0;
    do; while (i++ < 50);

    switch (1)
        ;

    i = ((0.5) ? 1 : 0);
    expecti(i, 1);

    // computed goto
    void *Q[] = { &&__a, &&__b, &&__c, &&__d };
    int _ = 0;
    goto *Q[0];
    _ = 100;

__a:
    if (_ == 5)
        return 0;
    expecti(_, 0);
    _ = 1;
    goto *Q[2];
    _ = 2;

__b:
    expecti(_, -1);
    _ = 5;
    goto *Q[3];
    _ = 0;

__c:
    expecti(_, 1);
    _ = -1;
    goto *Q[1];
    _ = 3;

__d:
    expecti(_, 5);
    goto **Q;


    return 1; // error
}
