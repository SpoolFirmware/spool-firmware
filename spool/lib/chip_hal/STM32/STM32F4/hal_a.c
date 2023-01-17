void halTest(void)
{
    volatile int *a = (volatile int *)0x1000;
    *a = 0x1;
}