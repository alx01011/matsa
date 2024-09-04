package java.util.concurrent.annotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

   /**
   * This annotation is used to mark classes that are known to be safe for use
   * with the ThreadSanitizer (TSan) tool. This annotation is used to suppress
   * false positive warnings from TSan.
   *
   * @since 17
   */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.FIELD)
public @interface MaTSaIgnoreField {}
