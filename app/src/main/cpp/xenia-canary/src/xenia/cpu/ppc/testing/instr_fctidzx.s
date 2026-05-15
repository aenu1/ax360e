test_fctidzx_1:
  #_ REGISTER_IN f1 3.7
  fctidz. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000003
  #_ REGISTER_OUT f1 3.7

test_fctidzx_2:
  #_ REGISTER_IN f1 -2.9
  fctidz. f3, f1
  blr
  #_ REGISTER_OUT f3 0xFFFFFFFFFFFFFFFE
  #_ REGISTER_OUT f1 -2.9

test_fctidzx_3:
  #_ REGISTER_IN f1 100.1
  fctidz. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000064
  #_ REGISTER_OUT f1 100.1

test_fctidzx_4:
  #_ REGISTER_IN f1 -0.5
  fctidz. f3, f1
  blr
  #_ REGISTER_OUT f3 0x0000000000000000
  #_ REGISTER_OUT f1 -0.5
