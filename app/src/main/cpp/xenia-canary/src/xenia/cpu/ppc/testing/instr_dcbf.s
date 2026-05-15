test_dcbf_1:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 0
  dcbf r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 0

test_dcbf_2:
  #_ REGISTER_IN r4 0x00001000
  #_ REGISTER_IN r5 64
  dcbf r4, r5
  blr
  #_ REGISTER_OUT r4 0x00001000
  #_ REGISTER_OUT r5 64

test_dcbf_3:
  #_ REGISTER_IN r4 0
  #_ REGISTER_IN r5 0x00001000
  dcbf r4, r5
  blr
  #_ REGISTER_OUT r4 0
  #_ REGISTER_OUT r5 0x00001000

test_dcbf_4:
  #_ REGISTER_IN r4 0x00002000
  #_ REGISTER_IN r5 128
  dcbf r4, r5
  blr
  #_ REGISTER_OUT r4 0x00002000
  #_ REGISTER_OUT r5 128
