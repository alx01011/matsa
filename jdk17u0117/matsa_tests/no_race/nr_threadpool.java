public class nr_threadpool {
    public static int x = 3;

    public static void main(String... args) {
        java.util.concurrent.ExecutorService executor = java.util.concurrent.Executors.newFixedThreadPool(2);

        executor.submit(() -> {
            increment();
        });

        executor.submit(() -> {
            increment();
        });

        executor.shutdown();

        try {
            executor.awaitTermination(1, java.util.concurrent.TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }

        System.out.println("OK");

    }

    public static synchronized void increment() {
        x++;
    }
}