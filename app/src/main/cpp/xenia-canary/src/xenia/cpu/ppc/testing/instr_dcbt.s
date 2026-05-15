test_dcbt_1:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 0
  dcbt r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 0

test_dcbt_2:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 64
  dcbt r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 64

test_dcbt_3:
  #_ REGISTER_IN r4 0
  #_ REGISTER_IN r5 0x00001000
  dcbt r4, r5
  blr
  #_ REGISTER_OUT r4 0
  #_ REGISTER_OUT r5 0x00001000

test_dcbt_4:
  #_ REGISTER_IN r4 0x00002000
  #_ REGISTER_IN r5 128
  dcbt r4, r5
  blr
  #_ REGISTER_OUT r4 0x00002000
  #_ REGISTER_OUT r5 128
