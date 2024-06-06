import java.util.concurrent.annotation.*;

public class nr_annotated_field {
    @JTsanIgnoreField
    public static int x;

    public static void main(String[] args) {
        // create 2 threads and form a race condition
        Thread t1 = new Thread(() -> {
            x = 42;
        });
        Thread t2 = new Thread(() -> {
            x = 43;
        });

        t1.start();
        t2.start();

        try {
            t1.join();
            t2.join();
        } catch (InterruptedException e) {
            System.out.println("Interrupted");
        }
    }
}
