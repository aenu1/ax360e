test_dcbtst_1:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 0
  dcbtst r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 0

test_dcbtst_2:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 64
  dcbtst r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 64

test_dcbtst_3:
  #_ REGISTER_IN r4 0
  #_ REGISTER_IN r5 0x00001000
  dcbtst r4, r5
  blr
  #_ REGISTER_OUT r4 0
  #_ REGISTER_OUT r5 0x00001000

test_dcbtst_4:
  #_ REGISTER_IN r4 0x00002000
  #_ REGISTER_IN r5 128
  dcbtst r4, r5
  blr
  #_ REGISTER_OUT r4 0x00002000
  #_ REGISTER_OUT r5 128
