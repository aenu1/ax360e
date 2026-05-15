test_fctiwzx_1:
  #_ REGISTER_IN f1 3.7
  fctiwz. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000003
  #_ REGISTER_OUT f1 3.7

test_fctiwzx_2:
  #_ REGISTER_IN f1 -2.9
  fctiwz. f3, f1
  blr
  #_ REGISTER_OUT f3 0xFFFFFFFFFFFFFFFE
  #_ REGISTER_OUT f1 -2.9

test_fctiwzx_3:
  #_ REGISTER_IN f1 100.1
  fctiwz. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000064
  #_ REGISTER_OUT f1 100.1

test_fctiwzx_4:
  #_ REGISTER_IN f1 -0.5
  fctiwz. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000000
  #_ REGISTER_OUT f1 -0.5
