## Running the Elevator Simulation

Follow these steps to compile and run the elevator simulation:
1.  **Clone The Repo**

2.  **Compile the helper program:**
    ```bash
    gcc helper-program.c -lpthread -o helper
    ```

3.  **Compile the solution:**
    ```bash
    gcc solution.c -lpthread -o solution
    ```

4.  **Execute the simulation:**
    Ensure that the test case file, the `helper` executable, and the `solution` executable are all located in the same directory. To run a specific test case, use the following command, replacing `<testcase-number-here>` with the desired test case number (from 1 to 12).

    ```bash
    ./helper <testcase-number-here>
    ```

    **Example:** To run test case number 1, execute:
    ```bash
    ./helper 1
    ```

## Troubleshooting "No space left on device" Error

If you encounter the following error:

No space left on device
Error in msgget creating message queue for solver 0: %

This typically indicates an issue with the number of message queues on your system. You can try the following steps to resolve it:

1.  **Install `util-linux` (if necessary):**
    ```bash
    brew install util-linux
    ```

2.  **Delete existing message queues:**
    ```bash
    for id in $(ipcs -q | awk 'NR > 3 { print $2 }'); do ipcrm -q $id; done
    ```

3.  **Try running the simulation again** using the steps outlined in the "Execute the simulation" section.

## Simulation Results (Macbook Pro M1 2020)

The following results were obtained by running the simulation on a Macbook Pro M1 (2020):

| Testcase | Time(sec) | Turns | Elevators Movements | Total Passengers |
| :------- | :-------- | :---- | :-----------------  | :--------------  |
| 1        | 0.006662  | 10    | 9                   | 3                | 
| 2        | 0.513595  | 237   | 1733                | 100              |
| 3        | 7.171142  | 1221  | 16813               | 250              |
| 4        | 10.756845 | 2168  | 33958               | 300              |
| 5        | 13.671850 | 1369  | 19708               | 400              | 
| 6        | 8.477230  | 1570  | 5199                | 200              |
| 7        | 26.287953 | 1408  | 11714               | 500              | 
| 8        | 17.449323 | 3334  | 14757               | 200              |
| 9        | 6.355263  | 634   | 8672                | 300              |
| 10       | 7.693402  | 1556  | 17781               | 250              | 
| 11       | 34.586539 | 2038  | 8998                | 400              |
| 12       | 21.710194 | 2826  | 37960               | 400              |

In 1 Turn we are moving the elevators in one position up or down or not moving it at all.
