module Mo where

sayHello :: String -> IO ()
sayHello x = putStrLn ("Hello, " ++ x ++ "!")

main :: IO ()
main = do
  putStrLn secondGreeting
  where secondGreeting = concat [hello, " ", world]
                         (++) "hello" " world!"


-- GHCi 
{-
    ghci
    Prelude> :l hello.hs
    Prelude> sayHello "x"
    Hello, x!
    Prelude> :q
-}

-- REPL read-eval-print loop
let triple x = x * 3 -- x->head of lambda x*3->body
-- Prelude> triple 2 // (triple x=x*3)2
-- 6
-- :m
-- Prelude> :i (+) (-)
-- infixl 6 +, -
-- Prelude> :t 'a'
-- 'a' :: Char
-- Prelude> :t "Hello!"
-- "Hello!" :: [Char]

x = 5
y = (1 -)
myResult = y x
-- -4

printInc n = print plusTwo
  where plusTwo = n + 2
printInc2 n = let plusTwo = n + 2
              in print plusTwo
printInc2' n =
  (\plusTwo -> print plusTwo) (n + 2)
c where a = b 
(\a -> c) b

data Bool = False | True

greetIfCool :: String -> IO ()
greetIfCool coolness = 
  if cool coolness
    then putStrLn "cool"
  else
    putStrLn "not cool"
  where cool v = v == "cool"

-- tuple (Integer, String)
let myTup = (1 :: Integer, "b")
fst myTup
-- -> 1
-- snd :: (a, b)
-- -> b
2 + fst (1, 2)
-- 3