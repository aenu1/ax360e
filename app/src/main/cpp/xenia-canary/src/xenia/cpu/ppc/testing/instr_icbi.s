test_icbi_1:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 0
  icbi r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 0

test_icbi_2:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 64
  icbi r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 64

test_icbi_3:
  #_ REGISTER_IN r4 0
  #_ REGISTER_IN r5 0x00001000
  icbi r4, r5
  blr
  #_ REGISTER_OUT r4 0
  #_ REGISTER_OUT r5 0x00001000

test_icbi_4:
  #_ REGISTER_IN r4 0x00002000
  #_ REGISTER_IN r5 128
  icbi r4, r5
  blr
  #_ REGISTER_OUT r4 0x00002000
  #_ REGISTER_OUT r5 128
