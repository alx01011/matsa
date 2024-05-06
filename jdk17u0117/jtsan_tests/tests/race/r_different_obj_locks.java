public class r_different_obj_locks {
    public static int x = 3;

    public static void main(String... args) {
        Object lock1 = new Object();
        Object lock2 = new Object();

        Thread t1 = new Thread(() -> {
            increment1(lock1);
        });

        Thread t2 = new Thread(() -> {
            increment2(lock2);
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

    public static void increment1(Object lock) {
        synchronized (lock) {
            x++;
        }
    }

    public static void increment2(Object lock) {
        synchronized (lock) {
            x++;
        }
    }
}