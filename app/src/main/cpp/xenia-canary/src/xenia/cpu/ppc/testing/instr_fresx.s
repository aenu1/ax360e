test_fresx_1:
  #_ REGISTER_IN f1 2.0
  fres. f3, f1
  blr
  #_ REGISTER_OUT f3 0.5
  #_ REGISTER_OUT f1 2.0

test_fresx_2:
  #_ REGISTER_IN f1 4.0
  fres. f3, f1
  blr
  #_ REGISTER_OUT f3 0.25
  #_ REGISTER_OUT f1 4.0

test_fresx_3:
  #_ REGISTER_IN f1 0.5
  fres. f3, f1
  blr
  #_ REGISTER_OUT f3 2.0
  #_ REGISTER_OUT f1 0.5

test_fresx_4:
  #_ REGISTER_IN f1 1.0
  fres. f3, f1
  blr
  #_ REGISTER_OUT f3 1.0
  #_ REGISTER_OUT f1 1.0
