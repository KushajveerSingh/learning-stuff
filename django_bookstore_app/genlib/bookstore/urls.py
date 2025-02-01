from django.urls import path
from django.contrib.auth import views as auth_views
from django.contrib import admin

from . import views

from django.conf.urls import url
from django.contrib import admin

admin.site.site_header = 'Genlib admin'
admin.site.site_title = 'Genlib admin'
admin.site.index_title = 'Genlib administration'
admin.empty_value_display = '**Empty**'

urlpatterns = [
    path('', views.index, name='index'),
    path('index.html', views.index),
    path('sign-up.html', views.signup, name='signup'),
    path('activate/<slug:uidb64>/<slug:token>/', views.activate, name='activate'),
    path('signup-confirmation.html', views.signup_confirmation, name='signup-confirmation'),
    path('login.html', views.login, name='login'),
    path('productDetails/<str:title>/', views.book_detail, name='book-details'),
    path('signout', views.signout, name='signout'),

    path('editprofile.html', views.edit_profile, name='edit_profile'),
    path('search.html', views.search, name='search'),
    path('browse-books.html', views.browse_books, name='browse_books'),
    path('cart.html', views.cart, name='cart'),
    path('orderHistory.html', views.order_history, name='order_history'),

    path('admin-home.html', views.admin_home, name='admin_home'),
    path('add_to_cart', views.add_to_cart, name='add_to_cart'),
    path('shipping.html', views.shipping, name='shipping'),
    path('payment.html', views.payment, name='payment'),
    path('finalplaceorder.html', views.finalplaceorder, name='finalplaceorder'),
    path('orderConfirmation.html', views.orderConfirmation, name='orderConfirmation'),

    path('password-reset/',
         auth_views.PasswordResetView.as_view(
             template_name='bookstore/password-reset/password_reset.html',
             subject_template_name='bookstore/password-reset/password_reset_subject.txt',
             email_template_name='bookstore/password-reset/password_reset_email.html',
         ),
         name='password_reset'),
    path('password-reset/done/',
         auth_views.PasswordResetDoneView.as_view(
             template_name='bookstore/password-reset/password_reset_done.html'
         ),
         name='password_reset_done'),
    path('password-reset-confirm/<uidb64>/<token>/',
         auth_views.PasswordResetConfirmView.as_view(
             template_name='bookstore/password-reset/password_reset_confirm.html',
         ),
         name='password_reset_confirm'),
    path('password-reset-complete/', views.password_reset_complete, name="password_reset_complete"),
    path(
        'change-password',
        auth_views.PasswordChangeView.as_view(
            template_name='bookstore/change_password.html',
            success_url='password-change-complete'
        ),
        name='change-password'
    ),
    path('password-change-complete', views.password_change_complete, name="pasword_change_complete")
]
