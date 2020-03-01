// line continuation

int \
main \
( \
    void // \
        \
    // one empty space to compensate
) { \
    int \
    a   \
    =   \
    0,  \
    *b  \
    =   \
    &a; \

    expecti(*b, a); // line continuation should make the above
                    // int a=0,*b=&a;

    /* \
     * \
     * \\ int *e=b; /
     */

    // would error if int *e made it as code
    int e = 0; // \
        comments  \
        comments  \
        e = 1;

    expecti(e, 0); // error if e = 1 made it

    return e;   // e == 0 (success)
}
