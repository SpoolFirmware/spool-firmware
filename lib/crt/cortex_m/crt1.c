extern char _sdata;
extern char _edata;
extern char _sidata;

extern char __bss_base__;
extern char __bss_end__;

void *memcpy(void *dst, void *src, unsigned int len);
void main(void);

void __copy_data(void)
{
    memcpy(&_sdata, &_sidata, &_edata - &_sdata);
    main();
}
