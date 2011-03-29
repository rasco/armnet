SECTIONS
{
  .text 0x22000000 :
  {
    __btext = .;
    *(.text)
    __etext = .;
  }
  .data :
  {
    __bdata = .;
    *(.data)
    __edata  =  .;
  }
  .bss :
  {
   *(.bss)
   *(COMMON)
   __end = . ;
  }
}

