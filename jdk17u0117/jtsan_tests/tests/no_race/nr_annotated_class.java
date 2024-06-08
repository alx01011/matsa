import java.util.concurrent.annotation.*;

@JTSanIgnoreClass
class ignore_this_class {
    public int x;
    public int y;
}

public class nr_annotated_class {
    public static void main(String... args) {
        ignore_this_class the_class = new ignore_this_class();
        // create 2 threads and form a race condition
        Thread t1 = new Thread(() -> {
            the_class.x = 2;
            the_class.y = 9;
        });
        Thread t2 = new Thread(() -> {
            the_class.x = 1;
            the_class.y = 3;
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
