# Automatic-Test-Pattern-Generation-for-Gate-Substitution-Errors

Please follow the following steps to execute the program and interpret the results.
1. Make sure you have the .lev file of the circuit for which the ATPG is to be generated in the same directory as of source code.
2. Make sure you have the gcc compiler installed.
3. You can then compile the code using g++ ATPG-for-GSE.cpp
4. After error free compilation, a.out file is generated.
4. Code then can be executed using ./a.out c17 or ./a.out c432
[Assuming the lev file placed in the directory is c17.lev or c432.lev]
5. The status for each fault, whether, it is detected or not along with the vector which detects that fault will be displayed on the console.
