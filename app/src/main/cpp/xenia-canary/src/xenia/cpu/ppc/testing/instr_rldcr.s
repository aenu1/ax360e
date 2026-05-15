.macro make_full_test_constant dest, a, b, c, d
  lis \dest, \a
  ori \dest, \dest, \b
  sldi \dest, \dest, 32
  lis r3, \c
  ori r3, r3, \d
  clrldi r3, r3, 32
  or \dest, \dest, r3
.endm

test_rldcr_1:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 24
  rldcr r3, r4, r5, 63
  blr
  #_ REGISTER_OUT r3 0x6789abcdef012345
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcr_1_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 24
  rldcr r3, r4, r5, 63
  blr
  #_ REGISTER_OUT r3 0x6789abcdef012345
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcr_2:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 24
  rldcr r3, r4, r5, 55
  blr
  #_ REGISTER_OUT r3 0x6789abcdef012300
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcr_2_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 24
  rldcr r3, r4, r5, 55
  blr
  #_ REGISTER_OUT r3 0x6789abcdef012300
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 24

test_rldcr_3:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 0
  rldcr r3, r4, r5, 63
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 0

test_rldcr_3_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 0
  rldcr r3, r4, r5, 63
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 0

test_rldcr_4:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 63
  rldcr r3, r4, r5, 63
  blr
  #_ REGISTER_OUT r3 0x8091a2b3c4d5e6f7
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 63

test_rldcr_4_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 63
  rldcr r3, r4, r5, 63
  blr
  #_ REGISTER_OUT r3 0x8091a2b3c4d5e6f7
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 63

test_rldcr_5:
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  #_ REGISTER_IN r5 32
  rldcr r3, r4, r5, 31
  blr
  #_ REGISTER_OUT r3 0x89abcdef00000000
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 32

test_rldcr_5_constant:
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  li r5, 32
  rldcr r3, r4, r5, 31
  blr
  #_ REGISTER_OUT r3 0x89abcdef00000000
  #_ REGISTER_OUT r4 0x0123456789ABCDEF
  #_ REGISTER_OUT r5 32
