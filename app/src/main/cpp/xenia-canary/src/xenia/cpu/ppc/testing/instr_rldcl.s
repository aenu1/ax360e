.macro make_full_test_constant dest, a, b, c, d
  lis \dest, \a
  ori \dest, \dest, \b
  sldi \dest, \dest, 32
  lis r3, \c
  ori r3, r3, \d
  clrldi r3, r3, 32
  or \dest, \dest, r3
.endm

test_rldcl_1:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 24
  rldcl r3, r4, r5, 0
  blr
  #_ REGISTER_OUT r3 0x6789abcdef012345
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcl_1_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 24
  rldcl r3, r4, r5, 0
  blr
  #_ REGISTER_OUT r3 0x6789abcdef012345
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcl_2:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 24
  rldcl r3, r4, r5, 8
  blr
  #_ REGISTER_OUT r3 0x0089abcdef012345
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcl_2_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 24
  rldcl r3, r4, r5, 8
  blr
  #_ REGISTER_OUT r3 0x0089abcdef012345
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcl_3:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 0
  rldcl r3, r4, r5, 0
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 0

test_rldcl_3_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 0
  rldcl r3, r4, r5, 0
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 0

test_rldcl_4:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 63
  rldcl r3, r4, r5, 0
  blr
  #_ REGISTER_OUT r3 0x8091a2b3c4d5e6f7
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 63

test_rldcl_4_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 63
  rldcl r3, r4, r5, 0
  blr
  #_ REGISTER_OUT r3 0x8091a2b3c4d5e6f7
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 63

test_rldcl_5:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 32
  rldcl r3, r4, r5, 32
  blr
  #_ REGISTER_OUT r3 0x0000000001234567
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 32

test_rldcl_5_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 32
  rldcl r3, r4, r5, 32
  blr
  #_ REGISTER_OUT r3 0x0000000001234567
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 32
