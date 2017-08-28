(define (reverse x) 
	(ifn x x
	(list (reverse(tail x)) (head x))))
