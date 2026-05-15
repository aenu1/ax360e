test_fnmsubsx_1:
  #_ REGISTER_IN f1 2.0
  #_ REGISTER_IN f2 3.0
  #_ REGISTER_IN f3 4.0
  fnmsubs. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 -2.0
  #_ REGISTER_OUT f1 2.0
  #_ REGISTER_OUT f2 3.0
  #_ REGISTER_OUT f3 4.0

test_fnmsubsx_2:
  #_ REGISTER_IN f1 -1.5
  #_ REGISTER_IN f2 2.0
  #_ REGISTER_IN f3 1.0
  fnmsubs. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 4.0
  #_ REGISTER_OUT f1 -1.5
  #_ REGISTER_OUT f2 2.0
  #_ REGISTER_OUT f3 1.0

test_fnmsubsx_3:
  #_ REGISTER_IN f1 0.5
  #_ REGISTER_IN f2 0.5
  #_ REGISTER_IN f3 0.25
  fnmsubs. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 0.0
  #_ REGISTER_OUT f1 0.5
  #_ REGISTER_OUT f2 0.5
  #_ REGISTER_OUT f3 0.25

test_fnmsubsx_4:
  #_ REGISTER_IN f1 10.0
  #_ REGISTER_IN f2 2.0
  #_ REGISTER_IN f3 5.0
  fnmsubs. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 -15.0
  #_ REGISTER_OUT f1 10.0
  #_ REGISTER_OUT f2 2.0
  #_ REGISTER_OUT f3 5.0
