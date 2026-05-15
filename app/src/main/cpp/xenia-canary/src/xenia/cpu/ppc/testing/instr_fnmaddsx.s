test_fnmaddsx_1:
  #_ REGISTER_IN f1 2.0
  #_ REGISTER_IN f2 3.0
  #_ REGISTER_IN f3 4.0
  fnmadds. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 -10.0
  #_ REGISTER_OUT f1 2.0
  #_ REGISTER_OUT f2 3.0
  #_ REGISTER_OUT f3 4.0

test_fnmaddsx_2:
  #_ REGISTER_IN f1 -1.5
  #_ REGISTER_IN f2 2.0
  #_ REGISTER_IN f3 1.0
  fnmadds. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 2.0
  #_ REGISTER_OUT f1 -1.5
  #_ REGISTER_OUT f2 2.0
  #_ REGISTER_OUT f3 1.0

test_fnmaddsx_3:
  #_ REGISTER_IN f1 0.5
  #_ REGISTER_IN f2 0.5
  #_ REGISTER_IN f3 0.25
  fnmadds. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 -0.5
  #_ REGISTER_OUT f1 0.5
  #_ REGISTER_OUT f2 0.5
  #_ REGISTER_OUT f3 0.25

test_fnmaddsx_4:
  #_ REGISTER_IN f1 0.0
  #_ REGISTER_IN f2 100.0
  #_ REGISTER_IN f3 5.0
  fnmadds. f5, f1, f2, f3
  blr
  #_ REGISTER_OUT f5 -5.0
  #_ REGISTER_OUT f1 0.0
  #_ REGISTER_OUT f2 100.0
  #_ REGISTER_OUT f3 5.0
