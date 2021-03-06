/ * * 
   * 	 @ f i l e 	 M O P S . c 
   * 	 @ d a t e 	 J a n   3 0 ,   2 0 1 6 
   * 	 @ a u t h o r 	 M i c h a l   O l e s z c z y k 
   * 	 @ b r i e f 	 F i l e   c o n t a i n i n g   f u n c t i o n s   r e s p o n s i b l e   f o r 
   * 	 	 	 c o m m u n i c a t i o n   b e t w e e n   M O P S   b r o k e r   a n d   l o c a l   p r o c e s s e s   a n d   f o r   g e n e r a l   b o r k e r   l o g i c . 
   * 
   * 	 I m p l e m e n t a t i o n   f o r   s e t   o f   f u n c t i o n s   f o r   b r o k e r - p r o c e s s   c o m m u n i c a t i o n 
   * 	 a n d   b r o k e r   l o g i c   i n   g e n e r a l .   C o m m u n i c a t i o n   i s   b a s e d   o n   q u e u e s   m e c h a n i s m . 
   * 	 E v e r y   p r o c e s s   i s   s e n d i n g   i t s   p r o c e s s   I D   t o   q u e u e   n a m e d   Q U E U E _ N A M E   ( o n   L i n u x   b a s e d   t a r g e t ) . 
   * / 
 # i n c l u d e   " M O P S . h " 
 # i n c l u d e   " M Q T T . h " 
 # i n c l u d e   " M O P S _ R T n e t _ C o n . h " 
 
 # i n c l u d e   " F r e e R T O S . h " 
 # i n c l u d e   " t i m e r s . h " 
 # i n c l u d e   " t a s k . h " 
 # i n c l u d e   " q u e u e . h " 
 # i n c l u d e   " s e m p h r . h " 
 
 # i n c l u d e   < s t d i n t . h > 
 # i n c l u d e   < u n i s t d . h > 
 # i n c l u d e   < s t d i o . h > 
 # i n c l u d e   < s t d l i b . h > 
 # i n c l u d e   < e r r n o . h > 
 # i n c l u d e   < s t r i n g . h > 
 # i n c l u d e   < s y s / t y p e s . h > 
 # i n c l u d e   < s y s / t i m e . h > 
 # i n c l u d e   < r t n e t . h > 
 # i n c l u d e   < r t m a c . h > 
 # i n c l u d e   < l i m i t s . h > 
 
 u i n t 8 _ t   * o u t p u t _ b u f f e r ; 	 	 	 	   	 	   	 / * * <   B u f f e r   f o r   s e n d i n g   d a t a   t o   R T n e t .   * / 
 
 S e m a p h o r e H a n d l e _ t   o u t p u t _ l o c k ; 	 	 	 / * * <   m u t e x   f o r   b l o c k i n g   a c c e s s   t o   # o u t p u t _ b u f f e r .   * / 
 S e m a p h o r e H a n d l e _ t   i n p u t _ l o c k ; 	 	 	 / * * <   m u t e x   f o r   b l o c k i n g   a c c e s s   t o   # i n p u t _ b u f f e r .   * / 
 S e m a p h o r e H a n d l e _ t   w a i t i n g _ o u t p u t _ l o c k ; 	 / * * <   m u t e x   f o r   b l o c k i n g   a c c e s s   t o   # w a i t i n g _ o u t p u t _ b u f f e r .   * / 
 S e m a p h o r e H a n d l e _ t   w a i t i n g _ i n p u t _ l o c k ; 	 / * * <   m u t e x   f o r   b l o c k i n g   a c c e s s   t o   # w a i t i n g _ i n p u t _ b u f f e r .   * / 
 M O P S _ Q u e u e   p r o c _ m o p s _ q u e u e ; 
 / /   * * * * * * * * * * * * * * *   G l o b a l   v a r i a b l e s   f o r   M O P S   b r o k e r   * * * * * * * * * * * * * * *   / / 
 
 / /   * * * * * * * * * * * * * * *       F u n t i o n s   f o r   l o c a l   p r o c e s s e s       * * * * * * * * * * * * * * * / / 
 
 / * * 
   *   @ b r i e f   F u n c t i o n   u s e d   i n   l o c a l   p r o c e s s e s   t o   c o n n e c t   t o   t h e   M O P S   b r o k e r . 
   * 
   *   F o r   R T n o d e   t a r g e t   c o m m u n i c a t i o n   M O P S   u s e s   M Q u e u e s   l i b r a r y   b u t   f o r   R T n o d e   - 
   *   Q u e u e   M a n a g e m e n t   m e c h a n i s m   ( u s e r   i n t e r f a c e   f u n c t i o n ) . 
   * 
   *   @ r e t u r n   0   -   i f   c o n n e c t i o n   s u c c e e d ,   1   -   i f   t h e r e   w a s   a   p r o b l e m   w i t h   c o n n e c t i o n . 
   * / 
 i n t   c o n n e c t T o M O P S ( )   { 
 
 	 w h i l e ( x R T n e t W a i t R e d y ( p o r t M A X _ D E L A Y )   = =   p d F A I L ) { ; } 
 	 v T a s k D e l a y ( 1 5 0 0 0 ) ; 
 	 p r o c _ m o p s _ q u e u e . M O P S T o P r o c e s _ f d   =   x Q u e u e C r e a t e ( M A X _ Q U E U E _ M E S S A G E _ N U M B E R ,   M A X _ Q U E U E _ M E S S A G E _ S I Z E ) ; 
 	 i f   ( 0   = =   p r o c _ m o p s _ q u e u e . M O P S T o P r o c e s _ f d ) { 
 	 	 p e r r o r ( " M Q u e u e   O p e n   M O P S T o P r o c e s " ) ; 
 	 	 r e t u r n   1 ; 
 	 } 
 
 	 p r o c _ m o p s _ q u e u e . P r o c e s T o M O P S _ f d   =   x Q u e u e C r e a t e ( M A X _ Q U E U E _ M E S S A G E _ N U M B E R ,   M A X _ Q U E U E _ M E S S A G E _ S I Z E ) ; 
 	 i f   ( 0   = =   p r o c _ m o p s _ q u e u e . P r o c e s T o M O P S _ f d ) { 
 	 	 p e r r o r ( " M Q u e u e   O p e n   P r o c e s T o M O P S " ) ; 
 	 	 r e t u r n   1 ; 
 	 } 
 	 x Q u e u e S e n d ( G l o b a l P r o c e s M o p s Q u e u e ,   ( v o i d * ) & p r o c _ m o p s _ q u e u e . M O P S T o P r o c e s _ f d ,   1 0 0 ) ; 
 	 x Q u e u e S e n d ( G l o b a l P r o c e s M o p s Q u e u e ,   ( v o i d * ) & p r o c _ m o p s _ q u e u e . P r o c e s T o M O P S _ f d ,   1 0 0 ) ; 
 	 v T a s k D e l a y ( 1 5 0 0 0 ) ; 
 	 r e t u r n   0 ; 
 } 
 
 / * * 
   *   @ b r i e f   S e n d s   i n d i c a t e d   b u f f e r   t o   c o n n e c t e d   M O P S   b r o k e r   ( l o w   l e v e l   f u n c t i o n ) . 
   * 
   *   @ p a r a m [ i n ]   b u f f e r   C o n t a i n s   d a t a   w h i c h   w i l l   b e   s e n t   t o   t h e   c o n n e c t e d   b r o k e r . 
   *   @ p a r a m [ i n ]   b u f f L e n   S p e c i f i e s   n u m b e r   o f   b y t e s   f r o m   b u f f e r   w h i c h   w i l l   b e   s e n t . 
   *   @ r e t u r n   0   i f   t h e   i t e m   w a s   s u c c e s s f u l l y   p o s t e d ,   o t h e r w i s e   - 1 . 
   * / 
 i n t   s e n d T o M O P S ( c h a r   * b u f f e r ,   u i n t 1 6 _ t   b u f f L e n )   { 
 	 w h i l e (   x Q u e u e S e n d ( p r o c _ m o p s _ q u e u e . P r o c e s T o M O P S _ f d ,   b u f f e r ,   0 )   ! =   p d T R U E   ) { ; } 
 	 r e t u r n   0 ; 
 } 
 
 / * * 
   *   @ b r i e f   R e c e i v e   d a t a   f r o m   M O P S   b r o k e r   ( l o w   l e v e l   f u n c t i o n ) . 
   * 
   *   @ p a r a m [ o u t ]   b u f f e r   C o n t a i n e r   f o r   d a t a   r e c e i v e d   f r o m   b r o k e r . 
   *   @ p a r a m [ i n ]   b u f f L e n   D e f i n e   n u m b e r   o f   b y t e s   w h i c h   c a n   b e   s t o r e d   i n   b u f f e r . 
   *   @ r e t u r n   M A X _ Q U E U E _ M E S S A G E _ S I Z E   i f   a n   i t e m   w a s   s u c c e s s f u l l y   r e c e i v e d   f r o m   t h e   q u e u e ,   o t h e r w i s e   b l o c k . 
   * / 
 i n t   r e c v F r o m M O P S ( c h a r   * b u f f e r ,   u i n t 1 6 _ t   b u f f L e n )   { 
 	 w h i l e   (   x Q u e u e R e c e i v e ( p r o c _ m o p s _ q u e u e . M O P S T o P r o c e s _ f d ,   b u f f e r ,   0 )   = =   p d F A L S E   ) 
 	 { ; } 
 	 r e t u r n     M A X _ Q U E U E _ M E S S A G E _ S I Z E ; 
 } 
 
 / /   * * * * * * * * * * * * * * *       F u n t i o n s   f o r   M O P S   b r o k e r       * * * * * * * * * * * * * * * / / 
 
 / * * 
   *   @ b r i e f   A d d i n g   n e w   l o c a l   p r o c e s s   t o   b r o k e r   c o m m u n i c a t i o n   q u e u e . 
   * 
   *   F o r   c o m m u n i c a t i o n   p r o c e s s < - > b r o k e r   a r e   u s e d   o n e   d i r e c t i o n   q u e u e s . 
   *   F i l e   d e s c r i p t o r s   f o r   t h a t   q u e u e s   ( 2   f o r   e a c h   p r o c e s s )   a r e   s t o r e d   i n 
   *   " c o m m u n i c a t i o n   l i s t " . 
   * 
   *   @ p a r a m [ i n ]   M O P S _ P r o c e s _ f d   F i l e   d e s c r i p t o r   f o r   a   q u e u e   M O P S - > p r o c e s s . 
   *   @ p a r a m [ i n ]   P r o c e s _ M O P S _ f d   F i l e   d e s c r i p t o r   f o r   a   q u e u e   p r o c e s s - > M O P S . 
   *   @ r e t u r n   C l i e n t   I D ,   w h i c h   i s   a l s o   i n d e x   o f   c o m m u n i c a t i o n   l i s t   i f 
   *   t h e r e   w a s   e n o u g h   p l a c e   t o   a d d   n e w   p r o c e s s   c o n n e c t i o n . \ n 
   *   - 1   -   i f   t h e r e   w a s   n o   p l a c e   t o   a d d   n e w   p r o c e s s   c o n n e c t i o n . 
   *   @ p o s t   O n e   p l a c e   i n   ' c o m m u n i c a t i o n   l i s t '   l e s s . 
   * / 
 i n t   A d d T o M O P S Q u e u e ( i n t   M O P S _ P r o c e s _ f d ,   i n t   P r o c e s _ M O P S _ f d )   { 
 	 i n t   i   =   0 ; 
 	 f o r   ( i   =   0 ;   i   <   M A X _ P R O C E S _ C O N N E C T I O N ;   i + + ) 
 	 	 i f   ( m o p s _ q u e u e [ i ] . M O P S T o P r o c e s _ f d   = =   0 
 	 	 	 	 & &   m o p s _ q u e u e [ i ] . P r o c e s T o M O P S _ f d   = =   0 )   { 
 	 	 	 m o p s _ q u e u e [ i ] . M O P S T o P r o c e s _ f d   =   ( Q u e u e H a n d l e _ t )   M O P S _ P r o c e s _ f d ; 
 	 	 	 m o p s _ q u e u e [ i ] . P r o c e s T o M O P S _ f d   =   ( Q u e u e H a n d l e _ t )   P r o c e s _ M O P S _ f d ; 
 	 	 	 r e t u r n   i ; 
 	 	 } 
 	 r e t u r n   - 1 ; 
 } 
 
 / / T O D O 
 / * * 
   *   @ b r i e f   M a i n   f u n c t i o n   f o r   s e t t i n g   p r o c e s s e s < - > b r o k e r   c o m m u n i c a t i o n . 
   * 
   *   T h i s   i s   p l a c e   w h e r e   i n i t i a l   q u e u e   p r o c e s s e s - > b r o k e r   i s   c r e a t e d . 
   *   B r o k e r   i s   l i s t e n i n g   o n   t h i s   q u e u e   a n d   i s   a d d i n g   n e w   c o n n e c t i o n s   t o   h i s 
   *   ' c o m m u n i c a t i o n   l i s t ' .   F u n c t i o n a l i t y   i s   b a s e d   o n   s e l e c t ( ) .   F u n c t i o n 
   *   i s   t a r g e t   s e n s i t i v e . 
   * 
   *   @ p o s t   T h i s   i s   b l o c k i n g   f u n c t i o n   ( n e v e r   e n d i n g   l o o p ) ! 
   * / 
 v o i d   I n i t P r o c e s C o n n e c t i o n ( )   { 
 
 	 Q u e u e H a n d l e _ t   n e w _ m q _ P r o c e s _ M O P S ; 
 	 s t a t i c   Q u e u e S e t H a n d l e _ t   m a s t e r ; 
 	 Q u e u e S e t M e m b e r H a n d l e _ t   x A c t i v a t e d M e m b e r ; 
 
 	 i f ( 0   ! =   m a s t e r ) { 
 	 	 r e t u r n ; 
 	 } 
 
 	 m a s t e r   =   x Q u e u e C r e a t e S e t (   M A X _ Q U E U E _ M E S S A G E _ S I Z E * s i z e o f ( Q u e u e H a n d l e _ t )   ) ; 
 
 	 G l o b a l P r o c e s M o p s Q u e u e   =   x Q u e u e C r e a t e ( M A X _ Q U E U E _ M E S S A G E _ N U M B E R ,   s i z e o f ( Q u e u e H a n d l e _ t ) ) ; 
 	 x Q u e u e A d d T o S e t (   G l o b a l P r o c e s M o p s Q u e u e ,   m a s t e r   ) ; 
 	 w h i l e ( 1 )   { 
 	 	 x A c t i v a t e d M e m b e r   =   x Q u e u e S e l e c t F r o m S e t (   m a s t e r ,   1 ) ; 
 	 	 i f   ( x A c t i v a t e d M e m b e r   ! =   N U L L )   {   / /   t h e r e   a r e   f i l e   d e s c r i p t o r s   t o   s e r v e 
 	 	 	 i f   ( x A c t i v a t e d M e m b e r   = =   G l o b a l P r o c e s M o p s Q u e u e )   { 
 	 	 	 	 n e w _ m q _ P r o c e s _ M O P S   =   S e r v e N e w P r o c e s s C o n n e c t i o n ( ) ; 
 	 	 	 	 x Q u e u e A d d T o S e t (   n e w _ m q _ P r o c e s _ M O P S ,   m a s t e r   ) ; 
 	 	 	 }   e l s e   { 
 	 	 	 	 R e c e i v e F r o m P r o c e s s ( ( i n t ) x A c t i v a t e d M e m b e r ) ; 
 	 	 	 } 
 	 	 } 
 	 } 
 } 
 
 / * * 
   *   @ b r i e f   R e c e i v i n g   d a t a   f r o m   c o n n e c t e d   l o c a l   p r o c e s s e s . 
   * 
   *   T h i s   i s   h i g h   l e v e l   f u n c t i o n   u s e d   f o r   r e a c t   w h e n   s e l e c t ( )   f u n c t i o n 
   *   r e t u r n   t h a t   f i l e   d e s c r i p t o r   ( f i l e _ d e   v a r i a b l e )   i f   r e a d y   t o   r e a d   s o m e   d a t a . 
   * 
   *   @ p a r a m [ i n ]   f i l e _ d e   F i l e   d e s c r i p t o r   o f   q u e u e   f r o m   w h i c h   d a t a   c a n   b e   r e a d . 
   *   @ r e t u r n   0   -   i n   e v e r y   c a s e   ( s t i l l   T O D O ) . 
   * / 
 i n t   R e c e i v e F r o m P r o c e s s ( i n t   f i l e _ d e )   { 
 	 i n t   C l i e n t I D ; 
 	 u i n t 8 _ t   t e m p [ M A X _ Q U E U E _ M E S S A G E _ S I Z E   +   1 ] ; 
 
 	 i f   ( x Q u e u e R e c e i v e ( f i l e _ d e ,   t e m p ,   0 )   = =   p d T R U E   )   { 
 	 	 C l i e n t I D   =   F i n d C l i e n t I D b y F i l e D e s c ( f i l e _ d e ) ; 
 	 	 A n a l y z e P r o c e s s M e s s a g e ( t e m p ,   M A X _ Q U E U E _ M E S S A G E _ S I Z E ,   C l i e n t I D ) ; 
 	 } 
 	 r e t u r n   0 ; 
 } 
 
 / * * 
   *   @ b r i e f   S e n d i n g   d a t a   f r o m   b r o k e r   t o   p a r t i c u l a r   f i l e   d e s c r i p t o r . 
   * 
   *   F u n c t i o n   s e n d s   b u f f e r   o f   g i v e n   l e n g t h   t o   g i v e n   f i l e   d e s c r i p t o r . 
   *   T h i s   i s   v e r y   l o w   l e v e l   f u n c t i o n .   I t   i s   t a r g e t   s e n s i t i v e . 
   * 
   *   @ p a r a m [ i n ]   b u f f e r   B u f f e r   o f   d a t a   t o   s e n d . 
   *   @ p a r a m [ i n ]   b u f f L e n   B u f f e r   l e n g t h . 
   *   @ p a r a m [ i n ]   f i l e _ d e   F i l e   d e s c r i p t o r ,   p l a c e   w h e r e   d a t a   s h o u l d   b e   s e n t . 
   *   @ r e t u r n   N u m b e r   o f   b y t e s   p r o p e r l y   s e n t . \ n 
   *   0   -   i f   q u e u e   i s   f u l l 
   * / 
 i n t   S e n d T o P r o c e s s ( u i n t 8 _ t   * b u f f e r ,   u i n t 1 6 _ t   b u f f L e n ,   i n t   f i l e _ d e )   { 
 	 i f   (   x Q u e u e S e n d ( f i l e _ d e ,   b u f f e r ,   0 )   = =   p d T R U E     ) 
 	 	 r e t u r n   M A X _ Q U E U E _ M E S S A G E _ S I Z E ; 
 	 r e t u r n   0 ; 
 } 
 
 / * * 
   *   @ b r i e f   M a i n   p l a c e   w h e r e   n e w   c o n n e c t i o n   f r o m   p r o c e s s e s   t o   b r o k e r   a r e   s e r v e . 
   * 
   *   F u n c t i o n   i s   f i r e d   w h e n   f i l e   d e s c r i p t o r s   o n   w h i c h   b r o k e r   i s   l i s t e n i n g   n e w 
   *   c o n n e c t i o n s   i s   s e t . 
   * 
   *   @ r e t u r n   F i l e   d e s c r i p t o r   v a l u e   -   w h e n   t h e r e   i s   p l a c e   i n   M O P S Q u e u e   a r r a y   ( ' c o n n e c t i o n   l i s t ' ) \ n 
   *   	 - 1   -   i f   t h e r e   i s   n o   p l a c e   i n   M O P S Q u e u e   a r r a y   o r   n o   m e s s a g e   r e c e i v e d   f r o m   l i s t e n e r _ f d 
   * / 
 Q u e u e H a n d l e _ t   S e r v e N e w P r o c e s s C o n n e c t i o n ( ) { 
 	 Q u e u e H a n d l e _ t   n e w _ m q _ P r o c e s _ M O P S ,   n e w _ m q _ M O P S _ P r o c e s ; 
 
 	 i f (   x Q u e u e R e c e i v e ( G l o b a l P r o c e s M o p s Q u e u e ,   & n e w _ m q _ M O P S _ P r o c e s ,   ( T i c k T y p e _ t ) 1 0 0 ) ) 
 	 	 i f (   x Q u e u e R e c e i v e ( G l o b a l P r o c e s M o p s Q u e u e ,   & n e w _ m q _ P r o c e s _ M O P S ,   ( T i c k T y p e _ t ) 1 0 0 ) ) 
 	 	 	 i f   ( A d d T o M O P S Q u e u e (   ( i n t ) n e w _ m q _ M O P S _ P r o c e s ,   ( i n t ) n e w _ m q _ P r o c e s _ M O P S )   > =   0 )   { 
 	 	 	 	 r e t u r n   n e w _ m q _ P r o c e s _ M O P S ; 
 	 	 	 } 
 	 r e t u r n   ( Q u e u e H a n d l e _ t )   - 1 ; 
 } 
 
 / * * 
   *   @ b r i e f   D e l e t i n g   n o t   n e e d e d   a n y m o r e   c o n n e c t i o n s   f r o m   ' c o n n e c t i o n   l i s t ' . 
   * 
   *   F i l e   d e s c r i p t o r s   s t o r e d   i n   ' c o n n e c t i o n   l i s t '   a r e   e r a s e d   ( s e t   t o   0 )   f o r 
   *   g i v e n   c l i e n t . 
   * 
   *   @ p a r a m [ i n ]   C l i e n t I D   I D   o f   c l i e n t   f o r   w h i c h   c o n n e c t i o n   s h o u l d   b e   c l o s e d . 
   *   @ p a r a m [ o u t ]   q u e u e   L i s t   o f   c o m m u n i c a t i o n   s t r u c t u r e   w h e r e   p a r t i c u l a r 
   *   c o m m u n i c a t i o n   w a s   s t o r e d . 
   *   @ p o s t   O n e   m o r e   f r e e   s p a c e   i n   ' c o m m u n i c a t i o n   l i s t ' . 
   * / 
 v o i d   D e l e t e P r o c e s s F r o m Q u e u e L i s t ( i n t   C l i e n t I D ,   M O P S _ Q u e u e   * q u e u e )   { 
 / / T O D O 
 } 
 
 / * * 
   *   @ b r i e f   W a i t s   f o r   T D M A   s y n c   s i g n a l   -   o v e r l a p s   w a i t O n T D M A S y n c   f o r   F r e e R T O S   R T n e t   p o r t 
   * 
   * / 
 u i n t 8 _ t   w a i t O n T D M A S y n c ( v o i d ) { 
 	 r e t u r n   x R T n e t W a i t S y n c ( p o r t M A X _ D E L A Y ) ; 
 } 
 
 / * * 
   *   @ b r i e f   I n i t i a l i z e s   R T n e t   c o n n e c t i o n 
   *   M a k e s   s o m e   t a r g e t   d e p e n d e n t   i n i t i a l i s a t i o n . 
   *   r e t u r n   1   i n   t h i s   c a s e   a l w a y s   f i x e d   v a l u e 
   * / 
 u i n t 8 _ t   R T n e t C o n n T a r g e t D e p e n d e n t I n i t ( v o i d ) { 
 	 o u t p u t _ b u f f e r   =   p v R T n e t G e t U d p D a t a B u f f e r ( U D P _ M A X _ S I Z E ) ; 
 	 r e t u r n   1 ; 
 } 
 
 / * * 
   *   @ b r i e f   T h i s   f u n c t i o n   n e e d s   t o   b e   i m p l e m e n t e d   f o r   e v e r y   p l a t o f o r m   
   *   y e t   i n   c a s e   o f   F r e e R T O S   i t   i s   e m p t y . 
   * / 
 v o i d   l o c k M e m o r y I n i t ( v o i d ) { } 
 
 / * * 
   *   @ b r i e f   T h i s   f u n c t i o n   n e e d s   t o   b e   i m p l e m e n t e d   f o r   e v e r y   p l a t o f o r m   
   *   y e t   i n   c a s e   o f   F r e e R T O S   i t   i s   e m p t y . 
   * / 
 v o i d   s t a r t R a n d o m G e n r a t o r ( v o i d ) { } 
 
 / * * 
   *   @ b r i e f   T h i s   f u n c t i o n   n e e d s   t o   b e   i m p l e m e n t e d   f o r   e v e r y   p l a t o f o r m   
   *   y e t   i n   c a s e   o f   F r e e R T O S   i t   i s   e m p t y . 
   * / 
 v o i d   M O P S B r o k e r T a r g e t I n i t ( v o i d ) { } 