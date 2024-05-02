public class nr_arrays {
    static double[] darr = new double[20];

    public static void main(String... args) {
        int[] iarr = new int[20];

        Thread t1 = new Thread(() -> {
            increment(iarr, 1);
            increment(iarr, 3);

            increment(darr, 1);
            increment(darr, 3);
        });

        Thread t2 = new Thread(() -> {
            increment(iarr, 2);
            increment(iarr, 4);

            increment(darr, 2);
            increment(darr, 4);
        });

        t1.start();
        t2.start();

        try {
            t1.join();
            t2.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }

    }

    public static void increment(int[] arr, int index) {
        arr[index]++;
    }

    public static void increment(double[] arr, int index) {
        arr[index]++;
    }
}