mappend [1..5][] = [1..5]
mappend x mempty = x
class Manoid m where
    mempty :: m
    mappend :: m -> m -> m
    mconcat :: [m] -> m
    mconcat = foldr mappend mempty
foldr (++) [] [[1..3], [4..6]] == [1,2,3,4,5,6]
instance Monoid [a] where
    mempty = []
    mappend = (++)