MEMORY
{
  ram : ORIGIN = 0x00000000, LENGTH = 32M
  rom : ORIGIN = 0x60000000, LENGTH = 8M
}

SECTIONS
{
  . = 0x00218000;

  text . : /* The actual instructions. */
  {
    _TextStart = .;
    *(.text)
    _TextEnd = .;
    *(.got)
    *(.got.plt)
    *(.rodata)
    *(.rodata.str1.4)
    *(.glue_7)
    *(.glue_7t)
  } >ram

  data . : /* Initialized data. */
  {
    _DataStart = . ;
    *(.data)
    *(.data.rel)
    _DataEnd = . ;
  } >ram

  bss . : /* Uninitialized data. */
  {
    _BssStart = . ;
    *(.bss)
    *(COMMON)
    _BssEnd = . ;
  } >ram

  kernel_mem . :
	{
		_KernelMemStart = .;
	} >ram
	
	_UserMemStart = 0x0300000;
	_UserMemEnd   = 0x1900000; 
}

ENTRY(main)
