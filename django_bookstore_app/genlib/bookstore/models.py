from django.db import models
from django.dispatch import receiver
from django.db.models.signals import post_save
from django.core.mail import send_mass_mail
import datetime
from django.contrib.auth.models import AbstractBaseUser, PermissionsMixin, BaseUserManager
from django_cryptography.fields import encrypt
from django.core.exceptions import ValidationError
from django.core.validators import MinValueValidator, MaxValueValidator
import uuid


class Book(models.Model):
    isbn = models.CharField(max_length=13, unique=True, primary_key=True)
    category = models.CharField(max_length=30)
    title = models.CharField(max_length=100)
    description = models.TextField()
    image_path = models.CharField(max_length=30)
    edition = models.IntegerField()
    publisher = models.CharField(max_length=30)
    publication_year = models.CharField(max_length=4)
    author = models.CharField(max_length=30)
    stock = models.IntegerField()
    cost = models.DecimalField(max_digits=4, decimal_places=2)
    rating = models.IntegerField(default=2)

    def __str__(self):
        return self.title


class UserManager(BaseUserManager):
    def create_user(self, first_name, last_name, email, password, 
                    receive_promotions, phone="", street="",
                    city="", state="", zip_code="", county="", country="", card_count=0, 
                    card_name1="", card_num1="", card_exp1="", card_cvv1="", card_four1="",
                    card_name2="", card_num2="", card_exp2="", card_cvv2="", card_four2="",
                    card_name3="", card_num3="", card_exp3="", card_cvv3="", card_four3="",
                    **kwargs):
        email = self.normalize_email(email)
        user = self.model(first_name=first_name,
                          last_name=last_name,
                          email=email,
                          password=password,
                          receive_promotions=receive_promotions,
                          phone=phone,
                          street=street,
                          city=city,
                          state=state,
                          zip_code=zip_code,
                          county=county,
                          country=country,
                          card_count=card_count,
                          card_name1=card_name1,
                          card_num1=card_num1,
                          card_exp1=card_exp1,
                          card_cvv1=card_cvv1,
                          card_four1=card_four1,
                          card_name2=card_name2,
                          card_num2=card_num2,
                          card_exp2=card_exp2,
                          card_cvv2=card_cvv2,
                          card_four2=card_four2,
                          card_name3=card_name3,
                          card_num3=card_num3,
                          card_exp3=card_exp3,
                          card_cvv3=card_cvv3,
                          card_four3=card_four3,
                          **kwargs)
        user.set_password(password)
        user.save()

    def create_superuser(self, first_name, last_name, email, password, 
                         receive_promotions, phone="", street="",
                         city="", state="", zip_code="", county="", country="", card_count=0, 
                         card_name1="", card_num1="", card_exp1="", card_cvv1="", card_four1="",
                         card_name2="", card_num2="", card_exp2="", card_cvv2="", card_four2="",
                         card_name3="", card_num3="", card_exp3="", card_cvv3="", card_four3="",
                         **kwargs):
        kwargs.setdefault('is_staff', True)
        kwargs.setdefault('is_superuser', True)
        kwargs.setdefault('is_active', True)
        kwargs.setdefault('is_suspended', False)

        return self.create_user(first_name=first_name,
                                last_name=last_name,
                                email=email,
                                password=password, 
                                receive_promotions=receive_promotions, 
                                phone=phone,
                                street=street,
                                city=city,
                                state=state,
                                zip_code=zip_code,
                                county=county,
                                country=country,
                                card_count=card_count,
                                card_name1=card_name1,
                                card_num1=card_num1,
                                card_exp1=card_exp1,
                                card_cvv1=card_cvv1,
                                card_four1=card_four1,
                                card_name2=card_name2,
                                card_num2=card_num2,
                                card_exp2=card_exp2,
                                card_cvv2=card_cvv2,
                                card_four2=card_four2,
                                card_name3=card_name3,
                                card_num3=card_num3,
                                card_exp3=card_exp3,
                                card_cvv3=card_cvv3,
                                card_four3=card_four3,
                                **kwargs)


class User(AbstractBaseUser, PermissionsMixin):
    first_name = models.CharField(max_length=30)
    last_name = models.CharField(max_length=30)
    email = models.EmailField(max_length=50, unique=True, primary_key=True)
    receive_promotions = models.BooleanField(default=False)

    phone = models.CharField(max_length=11, null=True, blank=True)
    street = models.CharField(max_length=100, null=True, blank=True)
    city = models.CharField(max_length=20, null=True, blank=True)
    state = models.CharField(max_length=20, null=True, blank=True)
    zip_code = models.CharField(max_length=5, null=True, blank=True)
    county = models.CharField(max_length=10, null=True, blank=True)
    country = models.CharField(max_length=10, null=True, blank=True)

    card_count = models.IntegerField(default=0)

    card_name1 = encrypt(models.CharField(max_length=100, null=True, blank=True))
    card_num1 = encrypt(models.CharField(max_length=16, null=True, blank=True))
    card_exp1 = encrypt(models.CharField(max_length=5, null=True, blank=True))
    card_cvv1 = encrypt(models.CharField(max_length=3, null=True, blank=True))
    card_four1 = models.CharField(max_length=4, default="", blank=True)

    card_name2 = encrypt(models.CharField(max_length=100, null=True, blank=True))
    card_num2 = encrypt(models.CharField(max_length=16, null=True, blank=True))
    card_exp2 = encrypt(models.CharField(max_length=5, null=True, blank=True))
    card_cvv2 = encrypt(models.CharField(max_length=3, null=True, blank=True))
    card_four2 = models.CharField(max_length=4, default="", blank=True)

    card_name3 = encrypt(models.CharField(max_length=100, null=True, blank=True))
    card_num3 = encrypt(models.CharField(max_length=16, null=True, blank=True))
    card_exp3 = encrypt(models.CharField(max_length=5, null=True, blank=True))
    card_cvv3 = encrypt(models.CharField(max_length=3, null=True, blank=True))
    card_four3 = models.CharField(max_length=4, default="", blank=True)

    is_staff = models.BooleanField(default=False)
    is_active = models.BooleanField(default=False)
    is_suspended = models.BooleanField(default=False)

    objects = UserManager()

    USERNAME_FIELD = 'email'
    REQUIRED_FIELDS = ['first_name', 'last_name', 'password', 'receive_promotions', 'phone']

    def __str__(self):
        return self.email


class Promotion(models.Model):
    code = models.CharField(max_length=10, primary_key=True)
    percentage = models.IntegerField()
    start_date = models.DateField(blank=True)
    end_date = models.DateField(blank=True)
    description = models.TextField(default="")

    def __str__(self):
        return self.code


@receiver(post_save, sender=Promotion)
def email_promotion(sender, instance, **kwargs):
    if instance.code == "SYSTEM":
        return
    mail_subject = "New promotion added on Genlib"
    mail_message = instance.description + "\nUse code: " + instance.code + "\nOffer expires: " + instance.end_date.strftime("%Y-%m-%d")
    mail_sender = "kushaj123456@gmail.com"
    mail_receivers = []

    for user in User.objects.all():
        if user.receive_promotions == True:
            mail_receivers.append(user.email)

    messages = [(mail_subject, mail_message, mail_sender, [recipient]) for recipient in mail_receivers]

    send_mass_mail(messages)


class CartItemManager(models.Manager):
    def add_cart_item(self, user, book, quantity):
        cart_item = self.create(user=user, book=book, quantity=quantity)
        return cart_item


class CartItem(models.Model):
    id = models.AutoField(primary_key=True)
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    book = models.ForeignKey(Book, on_delete=models.CASCADE)
    quantity = models.IntegerField()

    objects = CartItemManager()

    def __str__(self):
        return str(self.id)


class OrderManager(models.Manager):
    def create_order(self, user, total, orig_total, promotion, date, time, first_name, last_name,
                     phone, street, city, state, zip_code, county, country,
                     card_name, card_num, card_exp, card_cvv, card_four, status="Incomplete"):
        order = self.create(user=user,
                            total=total,
                            orig_total=orig_total,
                            promotion=promotion,
                            date=date,
                            time=time,
                            first_name=first_name,
                            last_name=last_name,
                            phone=phone,
                            street=street,
                            city=city,
                            state=state,
                            zip_code=zip_code,
                            county=county,
                            country=country,
                            card_name=card_name,
                            card_num=card_num,
                            card_exp=card_exp,
                            card_cvv=card_cvv,
                            card_four=card_four,
                            status=status)
        return order


class Order(models.Model):
    id = models.AutoField(primary_key=True)
    status = models.CharField(max_length=20, default="Incomplete") # Confirmed
    user = models.ForeignKey(User, on_delete=models.CASCADE)
    total = models.DecimalField(decimal_places=2, max_digits=5)
    promotion = models.ForeignKey(Promotion, on_delete=models.CASCADE, null=True)
    orig_total = models.DecimalField(decimal_places=2, max_digits=5, default=0)

    date = models.DateField()
    time = models.TimeField()

    first_name = models.CharField(max_length=30)
    last_name = models.CharField(max_length=30)

    phone = models.CharField(max_length=11)
    street = models.CharField(max_length=100)
    city = models.CharField(max_length=20)
    state = models.CharField(max_length=20)
    zip_code = models.CharField(max_length=5)
    county = models.CharField(max_length=10)
    country = models.CharField(max_length=10)

    card_name = encrypt(models.CharField(max_length=100))
    card_num = encrypt(models.CharField(max_length=16))
    card_exp = encrypt(models.CharField(max_length=5))
    card_cvv = encrypt(models.CharField(max_length=3))
    card_four = models.CharField(max_length=4)

    objects = OrderManager()

    def __str__(self):
        return str(self.user.email)


class OrderItemManager(models.Manager):
    def add_order_item(self, order, book, quantity):
        order_item = self.create(order=order, book=book, quantity=quantity)
        return order_item


class OrderItem(models.Model):
    id = models.AutoField(primary_key=True)
    order = models.ForeignKey(Order, on_delete=models.CASCADE)
    book = models.ForeignKey(Book, on_delete=models.CASCADE)
    quantity = models.IntegerField()

    objects = OrderItemManager()

    def __str__(self):
        return str(self.id)


class Search(models.Model):
    id = models.AutoField(primary_key=True)
    user = models.ForeignKey(User, on_delete=models.CASCADE, null=True)
    query = models.CharField(max_length=50, blank=True, null=True)
    is_cat = models.BooleanField(default=False)
    is_signedoff = models.BooleanField(default=False)

    def __str__(self):
        return str(self.user) + " " + str(self.query)
