public class nr_semaphore {
    public static java.util.concurrent.Semaphore semaphore = new java.util.concurrent.Semaphore(1);
    public static int x = 3;

    public static void main(String... args) {
        Thread t1 = new Thread(() -> {
            increment();
        });

        Thread t2 = new Thread(() -> {
            increment();
        });

        t1.start();
        t2.start();

        try {
            t1.join();
            t2.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }

        System.out.println("OK");

    }

    public static void increment() {
        try {
            semaphore.acquire();
            x++;
            semaphore.release();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}

