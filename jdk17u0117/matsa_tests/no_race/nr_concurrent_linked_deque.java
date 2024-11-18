public class nr_concurrent_linked_deque {
    public static void main(String... args) {
        java.util.concurrent.ConcurrentLinkedDeque<Integer> deque = new java.util.concurrent.ConcurrentLinkedDeque<>();
        deque.add(1);
        deque.add(2);

        Thread t1 = new Thread(() -> {
            increment(deque, 1);
        });

        Thread t2 = new Thread(() -> {
            increment(deque, 2);
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

    public static void increment(java.util.concurrent.ConcurrentLinkedDeque<Integer> deque, int key) {
        deque.add(deque.poll() + 1);
    }
}