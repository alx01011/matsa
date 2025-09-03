import java.util.concurrent.ForkJoinPool;

public class nr_forkjoin {
    public static int x;
    public static int y;

    public static void main(String... args) {
        x = 12;
        y = 13;

        // Create a ForkJoinPool with at least 2 threads
        ForkJoinPool pool = new ForkJoinPool(2);

        // Submit the first task to increment x
        var future1 = pool.submit(() -> increment1());

        // Submit the second task to increment y
        var future2 = pool.submit(() -> increment2());

        // Wait for both tasks to complete
        try {
            future1.get();
            future2.get();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

        // Shutdown the pool
        pool.shutdown();

        // After both increments
        x = 3;
        y = 4;
    }

    public static void increment1() {
        x++;
    }

    public static void increment2() {
        y++;
    }
}
