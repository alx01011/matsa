public class nr_concurrent_linked_queue {
    public static void main(String... args) {
        java.util.concurrent.ConcurrentLinkedQueue<Integer> queue = new java.util.concurrent.ConcurrentLinkedQueue<>();
        queue.add(1);
        queue.add(2);

        Thread t1 = new Thread(() -> {
            increment(queue, 1);
        });

        Thread t2 = new Thread(() -> {
            increment(queue, 2);
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

    public static void increment(java.util.concurrent.ConcurrentLinkedQueue<Integer> queue, int key) {
        queue.add(queue.poll() + 1);
    }
}