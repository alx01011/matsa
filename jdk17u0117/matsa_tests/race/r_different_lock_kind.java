public class r_different_lock_kind {
    // reentrant lock
    public static final java.util.concurrent.locks.ReentrantLock lock = new java.util.concurrent.locks.ReentrantLock();
    // semaphore
    public static java.util.concurrent.Semaphore semaphore = new java.util.concurrent.Semaphore(1);
    public static int x = 3;

    public static void main(String... args) {
        Thread t1 = new Thread(() -> {
            increment1();
        });

        Thread t2 = new Thread(() -> {
            increment2();
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

    public static void increment1() {
        lock.lock();
        x++;
        lock.unlock();
    }

    public static void increment2() {
        try {
            semaphore.acquire();
            x++;
            semaphore.release();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}