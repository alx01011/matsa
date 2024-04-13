import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class Race {
    private static int counter = 0;
    private static final Lock lock = new ReentrantLock();

    public static void main(String[] args) {
        Thread thread1 = new Thread(() -> {
            incrementCounter();
        });

	Thread thread2 = new Thread(() -> {
            incrementCounter();
        });

	thread1.start();
        thread2.start();

        try {
            thread1.join();
            thread2.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

	System.out.println("Final Counter Value: ");
    }

    private static void incrementCounter() {
//        lock.lock(); // Acquire the lock
            // Increment the counter safely within the locked region
            counter = 3;
//	lock.unlock();
    }
}

