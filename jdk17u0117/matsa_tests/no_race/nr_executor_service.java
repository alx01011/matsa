import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

public class nr_executor_service {
    public static int sharedResource = 0;

    public static void main(String... args) throws Exception {
        // A single-threaded executor ensures the task runs on a different thread.
        ExecutorService executor = Executors.newSingleThreadExecutor();

        // The main thread writes to the resource first.
        sharedResource = 1;

        Future<?> future = executor.submit(() -> {
            // The worker thread reads and then writes to the resource.
            System.out.println("Worker thread sees: " + sharedResource); // Should see 1
            sharedResource = 2;
        });

        // The get() call blocks the main thread until the task is complete.
        // This is the synchronization point.
        future.get();

        System.out.println(future.getClass());

        // This access should NOT be reported as a race.
        // It is guaranteed to happen after the task is finished.
        System.out.println("Main thread sees: " + sharedResource); // Should see 2
        sharedResource = 3;

        executor.shutdown();
        executor.awaitTermination(1, TimeUnit.MINUTES);
    }
}
