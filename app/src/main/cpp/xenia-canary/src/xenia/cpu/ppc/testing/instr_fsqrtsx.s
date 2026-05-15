test_fsqrtsx_1:
  #_ REGISTER_IN f1 4.0
  fsqrts. f3, f1
  blr
  #_ REGISTER_OUT f3 2.0
  #_ REGISTER_OUT f1 4.0

test_fsqrtsx_2:
  #_ REGISTER_IN f1 16.0
  fsqrts. f3, f1
  blr
  #_ REGISTER_OUT f3 4.0
  #_ REGISTER_OUT f1 16.0

test_fsqrtsx_3:
  #_ REGISTER_IN f1 1.0
  fsqrts. f3, f1
  blr
  #_ REGISTER_OUT f3 1.0
  #_ REGISTER_OUT f1 1.0

test_fsqrtsx_4:
  #_ REGISTER_IN f1 9.0
  fsqrts. f3, f1
  blr
  #_ REGISTER_OUT f3 3.0
  #_ REGISTER_OUT f1 9.0
