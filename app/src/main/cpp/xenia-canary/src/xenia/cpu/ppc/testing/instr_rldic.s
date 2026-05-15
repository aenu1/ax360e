.macro make_full_test_constant dest, a, b, c, d
  lis \dest, \a
  ori \dest, \dest, \b
  sldi \dest, \dest, 32
  lis r3, \c
  ori r3, r3, \d
  clrldi r3, r3, 32
  or \dest, \dest, r3
.endm

test_rldic_1:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  rldic r3, r4, 24, 8
  blr
  #_ REGISTER_OUT r3 0x0089abcdef000000
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_1_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  rldic r3, r4, 24, 8
  blr
  #_ REGISTER_OUT r3 0x0089abcdef000000
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_2:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  rldic r3, r4, 0, 0
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_2_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  rldic r3, r4, 0, 0
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_3:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  rldic r3, r4, 32, 32
  blr
  #_ REGISTER_OUT r3 0x89abcdef01234567
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_3_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  rldic r3, r4, 32, 32
  blr
  #_ REGISTER_OUT r3 0x89abcdef01234567
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_4:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  rldic r3, r4, 63, 1
  blr
  #_ REGISTER_OUT r3 0x8091a2b3c4d5e6f7
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_4_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  rldic r3, r4, 63, 1
  blr
  #_ REGISTER_OUT r3 0x8091a2b3c4d5e6f7
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldic_5:
  #_ REGISTER_IN r4 0xFFFFFFFFFFFFFFFF
  rldic r3, r4, 16, 48
  blr
  #_ REGISTER_OUT r3 0xffffffffffffffff
  #_ REGISTER_OUT r4 0xFFFFFFFFFFFFFFFF

test_rldic_5_constant:
  li r4, -1
  rldic r3, r4, 16, 48
  blr
  #_ REGISTER_OUT r3 0xffffffffffffffff
  #_ REGISTER_OUT r4 0xFFFFFFFFFFFFFFFF
