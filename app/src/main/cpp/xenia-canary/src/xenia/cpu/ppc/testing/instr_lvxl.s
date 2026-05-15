test_lvxl_1:
  # Basic aligned load with LRU hint
  #_ MEMORY_IN 0x0000000010001000 [01, 02, 03, 04, 05, 06, 07, 08, 09, 0A, 0B, 0C, 0D, 0E, 0F, 10]
  #_ REGISTER_IN r4 0x0000000010001000
  li r5, 0
  lvxl v3, r4, r5
  blr
  #_ REGISTER_OUT r4 0x0000000010001000
  #_ REGISTER_OUT r5 0
  #_ REGISTER_OUT v3 [00010200, 03000400, 05000600, 07000800]

test_lvxl_2:
  # Test with base+offset addressing
  #_ MEMORY_IN 0x0000000010001000 [00, 00, 00, 00, 00, 00, 00, 00, AA, BB, CC, DD, EE, FF, 11, 22]
  #_ MEMORY_IN 0x0000000010001010 [33, 44, 55, 66, 77, 88, 99, 00, 11, 22, 33, 44, 55, 66, 77, 88]
  #_ REGISTER_IN r4 0x0000000010001000
  li r5, 16
  lvxl v3, r4, r5
  blr
  #_ REGISTER_OUT r4 0x0000000010001000
  #_ REGISTER_OUT r5 16
  #_ REGISTER_OUT v3 [00034400, 55006600, 77008800, 99000000]

test_lvxl_3:
  # Test unaligned address (should mask to 16-byte boundary)
  #_ MEMORY_IN 0x0000000010001000 [11, 22, 33, 44, 55, 66, 77, 88, 99, AA, BB, CC, DD, EE, FF, 00]
  #_ REGISTER_IN r4 0x0000000010001000
  li r5, 7  # unaligned offset
  lvxl v3, r4, r5
  blr
  #_ REGISTER_OUT r4 0x0000000010001000
  #_ REGISTER_OUT r5 7
  # Should load from aligned address 0x10001000 (masks low 4 bits)
  #_ REGISTER_OUT v3 [00012200, 33004400, 55006600, 77008800]

test_lvxl_4:
  # Test with zero base register
  #_ MEMORY_IN 0x0000000010001000 [FF, EE, DD, CC, BB, AA, 99, 88, 77, 66, 55, 44, 33, 22, 11, 00]
  #_ REGISTER_IN r4 0
  lis r5, 0x1000
  addi r5, r5, 0x1000
  lvxl v3, r4, r5
  blr
  #_ REGISTER_OUT r4 0
  #_ REGISTER_OUT r5 0x10001000
  #_ REGISTER_OUT v3 [000FEE00, DD00CC00, BB00AA00, 99008800]

test_lvxl_5:
  # Test loading multiple vectors with LRU hints (cache behavior)
  #_ MEMORY_IN 0x0000000010001000 [AA, AA, AA, AA, BB, BB, BB, BB, CC, CC, CC, CC, DD, DD, DD, DD]
  #_ MEMORY_IN 0x0000000010001010 [11, 22, 33, 44, 55, 66, 77, 88, 99, AA, BB, CC, DD, EE, FF, 00]
  #_ MEMORY_IN 0x0000000010001020 [F0, E1, D2, C3, B4, A5, 96, 87, 78, 69, 5A, 4B, 3C, 2D, 1E, 0F]
  #_ REGISTER_IN r4 0x0000000010001000
  li r5, 0
  lvxl v3, r4, r5
  li r5, 16
  lvxl v4, r4, r5
  li r5, 32
  lvxl v5, r4, r5
  blr
  #_ REGISTER_OUT r4 0x0000000010001000
  #_ REGISTER_OUT r5 32
  #_ REGISTER_OUT v3 [000AAA00, AA00AA00, BB00BB00, BB00BB00]
  #_ REGISTER_OUT v4 [00012200, 33004400, 55006600, 77008800]
  #_ REGISTER_OUT v5 [0000E100, D200C300, B400A500, 96008700]

test_lvxl_6:
  # Verify lvxl behaves identically to lvx (LRU is just a cache hint)
  #_ MEMORY_IN 0x0000000010001000 [12, 34, 56, 78, 9A, BC, DE, F0, FE, DC, BA, 98, 76, 54, 32, 10]
  #_ REGISTER_IN r4 0x0000000010001000
  li r5, 0
  lvx v3, r4, r5   # First load with lvx
  lvxl v4, r4, r5  # Second load with lvxl - should be identical
  blr
  #_ REGISTER_OUT r4 0x0000000010001000
  #_ REGISTER_OUT r5 0
  #_ REGISTER_OUT v3 [00023400, 56007800, 9A00BC00, DE00F000]
  #_ REGISTER_OUT v4 [00023400, 56007800, 9A00BC00, DE00F000]
