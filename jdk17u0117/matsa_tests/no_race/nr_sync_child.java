public class nr_sync_child {
  long x;

  public void advance() {
	x++;
  }

  public synchronized void work() {
	advance();
	synchronized(this) {
		x = 8;
	}
  }

  public static void main(String... args) {
	nr_sync_child obj = new nr_sync_child();

        Thread t1 = new Thread(() -> {
		obj.work();
        });

        Thread t2 = new Thread(() -> {
		obj.work();
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

}
