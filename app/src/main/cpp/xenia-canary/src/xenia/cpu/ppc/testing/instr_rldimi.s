.macro make_full_test_constant dest, a, b, c, d
  lis \dest, \a
  ori \dest, \dest, \b
  sldi \dest, \dest, 32
  lis r3, \c
  ori r3, r3, \d
  clrldi r3, r3, 32
  or \dest, \dest, r3
.endm

test_rldimi_1:
  #_ REGISTER_IN r3 0xFFFFFFFFFFFFFFFF
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  rldimi r3, r4, 0, 0
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldimi_1_constant:
  li r3, -1
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  rldimi r3, r4, 0, 0
  blr
  #_ REGISTER_OUT r3 0x0123456789abcdef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldimi_2:
  #_ REGISTER_IN r3 0x0000000000000000
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  rldimi r3, r4, 8, 48
  blr
  #_ REGISTER_OUT r3 0x000000000000ef00
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldimi_2_constant:
  li r3, 0
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  rldimi r3, r4, 8, 48
  blr
  #_ REGISTER_OUT r3 0x0000000089abefef
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldimi_3:
  #_ REGISTER_IN r3 0xFFFFFFFFFFFFFFFF
  #_ REGISTER_IN r4 0x0123456789ABCDEF
  rldimi r3, r4, 32, 32
  blr
  #_ REGISTER_OUT r3 0x89ABCDEF01234567
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldimi_3_constant:
  li r3, -1
  make_full_test_constant r4, 0x0123, 0x4567, 0x89AB, 0xCDEF
  rldimi r3, r4, 32, 32
  blr
  #_ REGISTER_OUT r3 0x89ABCDEF01234567
  #_ REGISTER_OUT r4 0x0123456789ABCDEF

test_rldimi_4:
  #_ REGISTER_IN r3 0xAAAAAAAAAAAAAAAA
  #_ REGISTER_IN r4 0x00000000FFFFFFFF
  rldimi r3, r4, 16, 32
  blr
  #_ REGISTER_OUT r3 0xAAAAAAAAFFFFAAAA
  #_ REGISTER_OUT r4 0x00000000FFFFFFFF

test_rldimi_4_constant:
  li r3, -1
  srdi r3, r3, 1
  sldi r3, r3, 1
  li r4, -1
  clrldi r4, r4, 32
  rldimi r3, r4, 16, 32
  blr
  #_ REGISTER_OUT r3 0xFFFFFFFFFFFFFFFE
  #_ REGISTER_OUT r4 0x00000000FFFFFFFF

test_rldimi_5:
  #_ REGISTER_IN r3 0x0000000000000000
  #_ REGISTER_IN r4 0xFFFFFFFFFFFFFFFF
  rldimi r3, r4, 56, 0
  blr
  #_ REGISTER_OUT r3 0xFF00000000000000
  #_ REGISTER_OUT r4 0xFFFFFFFFFFFFFFFF

test_rldimi_5_constant:
  li r3, 0
  li r4, -1
  rldimi r3, r4, 56, 0
  blr
  #_ REGISTER_OUT r3 0xFF00000000000000
  #_ REGISTER_OUT r4 0xFFFFFFFFFFFFFFFF
