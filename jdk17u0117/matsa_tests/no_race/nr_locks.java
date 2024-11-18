import java.util.concurrent.locks.ReentrantLock;

public class nr_locks {
    public static int x = 3;
    public static int y = 4;

    // reentrant lock
    private static final ReentrantLock lock = new ReentrantLock();

    public static void main(String... args) {
        Thread t1 = new Thread(() -> {
            increment1();
        });

        Thread t2 = new Thread(() -> {
            increment1();
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

    public static void increment1() {
        lock.lock();
        x++;
        lock.unlock();
    }

    public static void increment2() {
        y++;
    }
}