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

    }

    public static void increment(int[] arr, int index) {
        arr[index]++;
    }
}