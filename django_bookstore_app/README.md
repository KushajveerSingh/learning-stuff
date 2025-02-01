# Genlib
- Developed on online bookstore application using Django and HTML/CSS/JavaScript, featuring secure user authentication, account activation, profile editing, and password management with automated email notifications to enhance user experience and security.
- Implemented advanced search functionality, allowing users to filter books by category, author/publisher, ratings, and price, alongside a seamless ordering process, including promotion code integration, order summary, and reordering options.
- Enabled admin capabilities to add and manage promotions, with automatic email notifications to opted-in users, while incorporating inventory checks for reordering and ensuring promotion validity, resulting in a comprehensive and user-friendly e-commerce platform.

## Table of Contents
- [Website working](#website-working)
    - [Logged off user](#logged-off-user)
    - [Sign-up process](#sign-up-process)
    - [Editing profile](#editing-profile)
    - [Change password](#change-password)
    - [Forgot password](#forgot-password)
    - [Searching for books](#searching-for-books)
    - [Placing an order](#placing-an-order)
    - [Other functionality](#other-functionality)
- [Database creation](#database-creation)
- [Requirements](#requirements)
- [Running the website](#running-the-website)
- [Email account to send emails](#email-account-to-send-emails)
- [Admin account](#admin-account)
- [Reset website](#reset-website)
- [License](#license)

## Website working

### Logged off user
A logged off user can search for books and look for the details of the interested books. When the user tries to make a purchase, the user is redirected to the sign-in page.

https://user-images.githubusercontent.com/24699564/134594909-bd846369-dd18-4fa0-b9f8-6139d0b98dae.mp4

### Sign-up process
To sign up, the user has to provide personal information and optionally, provide address and payment information. After a user has provided the required information, an email is sent to the user with a link to activate the account. Clicking the link activate's the account and redirect's the user to the homepage.

If the user has not activated the account, then the user would be treated as a logged-off user and when the user tries to sign-in, an error message, "Please activate your account" would be shown.

https://user-images.githubusercontent.com/24699564/134594932-3b85451c-6b0e-4258-ac48-14bb0f109970.mp4

### Editing profile
You can do the following
- Edit personal information
- Edit address information
- Add/Remove payment cards. A maximum of three cards can be stored in the system. All the card information is stored encrypted

When you click "save changes" the information is updated in the database and an email is sent to the user. The information sent includes
- Address has been deleted
- Any changes made to personal information
- Any changes made to address information
- If a new payment card has been added to your account
- If a previous payment card has been removed from your account

https://user-images.githubusercontent.com/24699564/134594948-862b50de-b33a-46c9-82e0-0ea7df37c65a.mp4

### Change password
A user can change the password of their account, if they are already logged in. To do this the user must provide the old password and then enter the new password. Checks are implemented to ensure the new password is strong, including things like
- New password cannot to similar to personal information
- New password cannot be similar to old password
- Checks about minimum length / special characters / numbers

An email is also sent to the user about the change the change of password.

https://user-images.githubusercontent.com/24699564/134594960-1f385e00-3c61-482b-9734-a006255180ea.mp4

### Forgot password
If a user cannot remember their password, then the user can click the *Forgot password* link on sign-in page, to reset their password.

Upon clicking the link, the user is asked to enter their email address. Then an email is sent to the user with a link that will take the user to the reset password page. Checks are implemented to ensure the new password is strong, including things like
- New password cannot to similar to personal information
- New password cannot be similar to old password
- Checks about minimum length / special characters / numbers

https://user-images.githubusercontent.com/24699564/134594972-37fe572c-a2b0-4758-9aa3-ea230a213d5b.mp4

### Searching for books
There are two types of search provided: basic search and advanced search.

In basic search, the user is required to type the name of a book in the search bar and if the name matches with a book in the database then the book is shown otherwise an error message that no book exists in the database is shown. For this project, we need to enter the exact name of the book (case insensitive).

In advanced search, we can filter the books using the following information
- Category
- Author / Publisher
- Top rated / Newest / Oldest / Price: Low to High / Price: High to Low

Using the above three filters, the user can search for the information they are interested in.

https://user-images.githubusercontent.com/24699564/134594987-9fe75a82-21f8-43b9-a0f0-069418255c44.mp4

### Placing an order
Steps to place an order are as follows
1. Add the respective books to the shopping cart.
2. After all the books have been added, go to your cart and then continue to checkout.
3. In the first step of checkout, you are required to enter your personal and address information. By default, these are fetched from your profile. Optionally, you can enter a valid promotion code to reduce the price of your order.
4. In the second step of checkout, you are required to enter the payment information. Here you are provided with a list of all the cards saved in your profile or you can enter a new payment card. Optionally, you can enter a valid promotion code to reduce the price of your order.
5. In the next step, a summary of your order is shown. And here you can also modify the information provided in step 3 and step 4 before finalizing the order.
6. Finally, you can place the order.

https://user-images.githubusercontent.com/24699564/134595004-c8db1fc4-dd12-4fac-b0bc-bdfcc4851024.mp4

### Adding promotions to website
An admin can add new promotions to be used by the users. When an admin adds a new promotion, an email is sent to all the users, who have opted in to receive promotions.

Now the users can use this promotion when they place their next order. All the required checks have implemented, including
- User can use the same promotion only once
- Promotion should have started before use
- Promotion should not have expired on use

https://user-images.githubusercontent.com/24699564/134595010-e2abe7ad-bd9b-4fbf-9a91-ce0c2757987a.mp4

### Other functionality
- You can reorder books from your previous orders. In case some books from your previous order are out of stock, then an error message is shown that some books could not be added to your cart as they are out of stock and the remaining books are added to the cart.
- View order history.

## Database creation
Database creation was a problem, as most of the websites don't allow you to scrape their websites. As a workaround I found the following [website](https://www.audiobooksnow.com/browse/) that had all the information about the books I was interested in.

So to scrape the website, manually copy the HTML code of the category of the book you are interested in and then run the code provided at this [link](https://gist.github.com/KushajveerSingh/9eef4b308a7284375f0fedd64841bb28).

Instructions regarding the code
- Name the HTML file as `temp.html` and place it in the same directory as the jupyter notebook
- All the images would be saved in `path = "cover_images"`
- The name of the images is a counter, starting from 100, `counter = 100` (e.g. 100.jpg, 101.jpg)

A backup of the database I used is also provided at [extra/database.zip](extra/database.zip).

## Requirements
The website was built using the following versions of the libraries
- python = 3.9.7
- Django = 3.2.7
- django-cryptography = 1.0

Download python using [Miniconda](https://docs.conda.io/en/latest/miniconda.html) and then install [Django](https://www.djangoproject.com/) using the following command

```
python3 -m pip install Django==3.2.7 django-cryptography==1.0
```

## Running the website
First clone the repository
```
git clone https://github.com/KushajveerSingh/genlib.git
cd genlib
```

Now you can run the website from the `genlib` folder
```
cd genlib
python manage.py runserver
```

The server will start at `localhost:8000`.

## Email account to send emails
For this project, I created a dummy gmail account. The credentials for this account are already setup in the `genlib/genlib/settings.py` as following
```
EMAIL_HOST_USER = 'kushaj123456@gmail.com'
EMAIL_HOST_PASSWORD = 'kushaj12345678'
```

Change the above settings with your own email and password, and **do not push these credentials to github**. If you decide to use my email, then it might not work as I think Google automatically turns off access to insecure apps (yes Django is insecure according to Gmail).

## Admin account
The default admin account has the following credentials
```
Email = kushaj@gmail.com
Password = 12345678
```

You can sign-in using the above credentials and you would automatically be taken to the admin portal, or you can navigate to `localhost:8000/admin` and then enter the above credentials to enter admin portal.

## Reset website
If you want to use this website, then you might want to remove all the previous stuff. Or if you run into some weird issue then you can also these instructions to reset everything.

1. Delete `genlib/db.sqlite3`
2. Delete `genlib/bookstore/migrations` folder
3. Migrate

    ```
    cd genlib
    python manage.py makemigrations bookstore
    python mange.py migrate
    ```
4. Create new admin account. Run the below code and then enter the credentials of the admin in the prompts.

    ```
    cd genlib
    python manage.py createsuperuser
    ```

## License
[Apache License v2](LICENSE)
