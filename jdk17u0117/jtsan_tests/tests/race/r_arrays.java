public class r_arrays {
    public static void main(String... args) {
        int[] arr = new int[20];

        Thread t1 = new Thread(() -> {
            increment(arr, 0);
            increment(arr, 3);
        });

        Thread t2 = new Thread(() -> {
            increment(arr, 0);
            increment(arr, 4);
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
}