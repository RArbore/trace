;;; Directory Local Variables
;;; For more information see (info "(emacs) Directory Variables")

((nil . (
	 (eval . (let
		     ((root
		       (projectile-project-root)))
		   (setq-local flycheck-clang-args
			       (list
				(concat "-I" root "lib/imgui") (concat "-std=c++20")))
		   (setq-local flycheck-clang-include-path
			       (list
				(concat root "lib/imgui")))
		   (setq-local flycheck-gcc-args
			       (list
				(concat "-I" root "lib/imgui") (concat "-std=c++20")))
		   (setq-local flycheck-gcc-include-path
			       (list
				(concat root "lib/imgui")))
		   ))))
 (c++-mode . ((c-basic-offset . 4)))
 (c-mode . ((mode . c++))))
