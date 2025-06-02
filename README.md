2025.6.2修复协程重入bug：hook->do_io里add_event后，yield前，另一个线程对同一个事件trigger_event，导致中断重入，解决：给协程加锁
