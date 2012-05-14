ENTRY (main)

MEMORY
{
  ram : ORIGIN = 0x00000000, LENGTH = 32M
  rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
  . = 0x00218000;

  .text . : /* The actual instructions. */
  {
    *(.text)
      *(.got)
      *(.got.plt)
      *(.rodata)
      *(.glue_7)
      *(.glue_7t)
  } >ram

  .data BLOCK (0x4) : /* Initialized data. */
  {
    _DataStart = . ;
    *(.data)
    _DataEnd = . ;
  } >ram

  .bss BLOCK (0x4) : /* Uninitialized data. */
  {
    _BssStart = . ;
    *(.bss)
    *(COMMON)
    _BssEnd = . ;
    _end =  ALIGN (0x8);
    __end = _end;
  } >ram
}
