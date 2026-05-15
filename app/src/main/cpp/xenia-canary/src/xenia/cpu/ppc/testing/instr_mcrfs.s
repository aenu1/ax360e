test_mcrfs_1:
  #_ REGISTER_IN f1 1.0
  #_ REGISTER_IN f2 2.0
  fadds f3, f1, f2
  mcrfs 1, 0
  blr
  #_ REGISTER_OUT f1 1.0
  #_ REGISTER_OUT f2 2.0
  #_ REGISTER_OUT f3 3.0

test_mcrfs_2:
  #_ REGISTER_IN f1 0x7FF0000000000001
  #_ REGISTER_IN f2 1.0
  fadd f3, f1, f2
  mcrfs 2, 0
  blr
  #_ REGISTER_OUT f1 0x7FF0000000000001
  #_ REGISTER_OUT f2 1.0
  #_ REGISTER_OUT f3 0x7FF8000000000001
