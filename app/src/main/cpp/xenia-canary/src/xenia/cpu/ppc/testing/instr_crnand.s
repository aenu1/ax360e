test_crnand_1:
  #_ REGISTER_IN r3 1
  #_ REGISTER_IN r4 2
  #_ REGISTER_IN r5 3
  #_ REGISTER_IN r6 4
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crnand 2, 0, 4
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 2
  #_ REGISTER_OUT r5 3
  #_ REGISTER_OUT r6 4
  #_ REGISTER_OUT r12 0x88000000

test_crnand_1_constant:
  li r3, 1
  li r4, 2
  li r5, 3
  li r6, 4
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crnand 2, 0, 4
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 2
  #_ REGISTER_OUT r5 3
  #_ REGISTER_OUT r6 4
  #_ REGISTER_OUT r12 0x88000000

test_crnand_2:
  #_ REGISTER_IN r3 2
  #_ REGISTER_IN r4 1
  #_ REGISTER_IN r5 4
  #_ REGISTER_IN r6 3
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crnand 2, 1, 5
  mfcr r12
  blr
  #_ REGISTER_OUT r3 2
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 4
  #_ REGISTER_OUT r6 3
  #_ REGISTER_OUT r12 0x44000000

test_crnand_2_constant:
  li r3, 2
  li r4, 1
  li r5, 4
  li r6, 3
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crnand 2, 1, 5
  mfcr r12
  blr
  #_ REGISTER_OUT r3 2
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 4
  #_ REGISTER_OUT r6 3
  #_ REGISTER_OUT r12 0x44000000

test_crnand_3:
  #_ REGISTER_IN r3 1
  #_ REGISTER_IN r4 1
  #_ REGISTER_IN r5 2
  #_ REGISTER_IN r6 2
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crnand 2, 2, 6
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 2
  #_ REGISTER_OUT r6 2
  #_ REGISTER_OUT r12 0x02000000

test_crnand_3_constant:
  li r3, 1
  li r4, 1
  li r5, 2
  li r6, 2
  cmpw cr0, r3, r4
  cmpw cr1, r5, r6
  crnand 2, 2, 6
  mfcr r12
  blr
  #_ REGISTER_OUT r3 1
  #_ REGISTER_OUT r4 1
  #_ REGISTER_OUT r5 2
  #_ REGISTER_OUT r6 2
  #_ REGISTER_OUT r12 0x02000000

test_crnand_4:
  #_ REGISTER_IN r3 5
  #_ REGISTER_IN r4 5
  #_ REGISTER_IN r5 10
  #_ REGISTER_IN r6 10
  cmpw cr2, r3, r4
  cmpw cr3, r5, r6
  crnand 10, 10, 14
  mfcr r12
  blr
  #_ REGISTER_OUT r3 5
  #_ REGISTER_OUT r4 5
  #_ REGISTER_OUT r5 10
  #_ REGISTER_OUT r6 10
  #_ REGISTER_OUT r12 0x00020000

test_crnand_4_constant:
  li r3, 5
  li r4, 5
  li r5, 10
  li r6, 10
  cmpw cr2, r3, r4
  cmpw cr3, r5, r6
  crnand 10, 10, 14
  mfcr r12
  blr
  #_ REGISTER_OUT r3 5
  #_ REGISTER_OUT r4 5
  #_ REGISTER_OUT r5 10
  #_ REGISTER_OUT r6 10
  #_ REGISTER_OUT r12 0x00020000

test_crnand_5:
  #_ REGISTER_IN r3 100
  #_ REGISTER_IN r4 50
  #_ REGISTER_IN r5 10
  #_ REGISTER_IN r6 20
  cmpw cr4, r3, r4
  cmpw cr5, r5, r6
  crnand 17, 17, 20
  mfcr r12
  blr
  #_ REGISTER_OUT r3 100
  #_ REGISTER_OUT r4 50
  #_ REGISTER_OUT r5 10
  #_ REGISTER_OUT r6 20
  #_ REGISTER_OUT r12 0x00000800

test_crnand_5_constant:
  li r3, 100
  li r4, 50
  li r5, 10
  li r6, 20
  cmpw cr4, r3, r4
  cmpw cr5, r5, r6
  crnand 17, 17, 20
  mfcr r12
  blr
  #_ REGISTER_OUT r3 100
  #_ REGISTER_OUT r4 50
  #_ REGISTER_OUT r5 10
  #_ REGISTER_OUT r6 20
  #_ REGISTER_OUT r12 0x00000800
